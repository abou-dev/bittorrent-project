#include "mbt/file/piece.h"

#include <errno.h>
#include <fcntl.h>
#include <mbt/be/torrent.h>
#include <mbt/be/torrent_files.h>
#include <mbt/be/torrent_getters.h>
#include <mbt/utils/str.h>
#include <mbt/utils/view.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mbt/file/file_handler.h"

struct mbt_piece
{
    size_t size;
    char *data;
    size_t nb_blocks;
    bool *received_blocks;
};

struct mbt_file_handler
{
    struct mbt_torrent *torrent;
    struct mbt_piece *pieces;
    size_t nb_pieces;
    struct mbt_cview pieces_hash; // Hash des pièces
};

void mbt_piece_dtor(struct mbt_piece *p)
{
    if (p == NULL)
        return;
    free(p->data);
    free(p->received_blocks);
    p->data = NULL;
    p->received_blocks = NULL;
}

size_t mbt_piece_get_nb_blocks(struct mbt_file_handler *fh, size_t piece_index)
{
    if (piece_index >= fh->nb_pieces)
        return 0;
    return fh->pieces[piece_index].nb_blocks;
}

const char *mbt_piece_get_data(struct mbt_file_handler *fh, size_t piece_index)
{
    if (piece_index >= fh->nb_pieces)
        return NULL;
    return fh->pieces[piece_index].data;
}

void mbt_piece_set_data(struct mbt_file_handler *fh, size_t piece_index,
                        const char *data, size_t size)
{
    if (piece_index >= fh->nb_pieces)
        return;
    struct mbt_piece *p = &fh->pieces[piece_index];
    if (size > p->size)
        size = p->size;
    memcpy(p->data, data, size);
}

bool mbt_piece_block_is_received(struct mbt_file_handler *fh,
                                 size_t piece_index, size_t block_index)
{
    if (piece_index >= fh->nb_pieces)
        return false;
    struct mbt_piece *p = &fh->pieces[piece_index];
    if (block_index >= p->nb_blocks)
        return false;
    return p->received_blocks[block_index];
}

void mbt_piece_block_set_received(struct mbt_file_handler *fh,
                                  size_t piece_index, size_t block_index,
                                  bool received)
{
    if (piece_index >= fh->nb_pieces)
        return;
    struct mbt_piece *p = &fh->pieces[piece_index];
    if (block_index >= p->nb_blocks)
        return;
    p->received_blocks[block_index] = received;
}

bool mbt_compare_hashes(struct mbt_file_handler *fh, size_t piece_index,
                        const char *received_data)
{
    if (piece_index >= fh->nb_pieces || received_data == NULL)
        return false;

    unsigned char computed_hash[SHA_DIGEST_LENGTH];
    size_t size = fh->pieces[piece_index].size;
    SHA1((const unsigned char *)received_data, size, computed_hash);

    struct mbt_cview pieces_hash_view = mbt_torrent_pieces(fh->torrent);
    const unsigned char *expected_hash =
        (const unsigned char *)(pieces_hash_view.data
                                + piece_index * SHA_DIGEST_LENGTH);

    return (memcmp(computed_hash, expected_hash, SHA_DIGEST_LENGTH) == 0);
}

enum mbt_piece_status mbt_piece_check(struct mbt_file_handler *fh,
                                      size_t piece_index)
{
    if (piece_index >= fh->nb_pieces)
        return MBT_PIECE_INVALID;
    struct mbt_piece *p = &fh->pieces[piece_index];

    for (size_t i = 0; i < p->nb_blocks; i++)
    {
        if (!p->received_blocks[i])
            return MBT_PIECE_DOWNLOADING;
    }

    if (mbt_compare_hashes(fh, piece_index, p->data))
        return MBT_PIECE_VALID;
    else
        return MBT_PIECE_INVALID;
}

bool mbt_piece_write(struct mbt_file_handler *fh, size_t piece_index)
{
    if (piece_index >= fh->nb_pieces)
        return false;
    struct mbt_piece *p = &fh->pieces[piece_index];

    size_t piece_length = mbt_torrent_piece_length(fh->torrent);
    size_t total_size = mbt_torrent_length(fh->torrent);

    size_t piece_offset = piece_index * piece_length;
    if (piece_offset + p->size > total_size)
        p->size = total_size - piece_offset;

    size_t remaining_size = p->size;
    size_t data_offset = 0;

    size_t nb_files = mbt_torrent_files_size(fh->torrent);
    size_t cumulative_size = 0;

    for (size_t i = 0; i < nb_files && remaining_size > 0; i++)
    {
        const struct mbt_torrent_file *file =
            mbt_torrent_files_get(fh->torrent, i);
        size_t file_length = mbt_torrent_file_length(file);
        size_t file_start = cumulative_size;
        size_t file_end = cumulative_size + file_length;

        if (piece_offset < file_end && piece_offset + p->size > file_start)
        {
            size_t file_offset = 0;
            if (piece_offset > file_start)
                file_offset = piece_offset - file_start;

            size_t write_size = remaining_size;
            if (file_offset + write_size > file_length)
                write_size = file_length - file_offset;

            size_t path_size = mbt_torrent_file_path_size(file);
            struct mbt_str path_str;
            if (!mbt_str_ctor(&path_str, 0))
            {
                fprintf(stderr, "Failed to initialize path string.\n");
                return false;
            }

            for (size_t j = 0; j < path_size; j++)
            {
                struct mbt_cview path_component =
                    mbt_torrent_file_path_get(file, j);
                if (!mbt_str_pushcv(&path_str, path_component))
                {
                    fprintf(stderr, "Failed to append path component.\n");
                    mbt_str_dtor(&path_str);
                    return false;
                }
                if (j < path_size - 1)
                    mbt_str_pushc(&path_str, '/');
            }

            char *dir_path = strdup(path_str.data);
            if (dir_path == NULL)
            {
                fprintf(stderr, "Failed to duplicate path string.\n");
                mbt_str_dtor(&path_str);
                return false;
            }
            char *last_slash = strrchr(dir_path, '/');
            if (last_slash)
            {
                *last_slash = '\0';
                char mkdir_cmd[512];
                snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p \"%s\"",
                         dir_path);
                system(mkdir_cmd);
            }
            free(dir_path);

            int fd = open(path_str.data, O_WRONLY | O_CREAT, 0644);
            mbt_str_dtor(&path_str);

            if (fd == -1)
            {
                perror("open failed");
                return false;
            }

            if (lseek(fd, file_offset, SEEK_SET) == (off_t)-1)
            {
                perror("lseek failed");
                close(fd);
                return false;
            }

            ssize_t written = write(fd, p->data + data_offset, write_size);
            if (written != (ssize_t)write_size)
            {
                perror("write failed");
                close(fd);
                return false;
            }

            close(fd);

            remaining_size -= write_size;
            data_offset += write_size;
            piece_offset += write_size;
        }

        cumulative_size += file_length;
    }

    if (remaining_size > 0)
    {
        fprintf(stderr, "Could not write entire piece %zu.\n", piece_index);
        return false;
    }

    return true;
}
