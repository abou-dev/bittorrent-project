#include "mbt/file/piece.h"

#include <errno.h>
#include <fcntl.h>
#include <mbt/be/torrent.h>
#include <mbt/utils/str.h>
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
    if (fh == NULL || piece_index >= fh->nb_pieces)
        return 0;
    return fh->pieces[piece_index].nb_blocks;
}

const char *mbt_piece_get_data(struct mbt_file_handler *fh, size_t piece_index)
{
    if (fh == NULL || piece_index >= fh->nb_pieces)
        return NULL;
    return fh->pieces[piece_index].data;
}

void mbt_piece_set_data(struct mbt_file_handler *fh, size_t piece_index,
                        const char *data, size_t size)
{
    if (fh == NULL || piece_index >= fh->nb_pieces || data == NULL)
        return;
    struct mbt_piece *p = &fh->pieces[piece_index];
    if (size > p->size)
        size = p->size;
    memcpy(p->data, data, size);
}

bool mbt_piece_block_is_received(struct mbt_file_handler *fh,
                                 size_t piece_index, size_t block_index)
{
    if (fh == NULL || piece_index >= fh->nb_pieces)
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
    if (fh == NULL || piece_index >= fh->nb_pieces)
        return;
    struct mbt_piece *p = &fh->pieces[piece_index];
    if (block_index >= p->nb_blocks)
        return;
    p->received_blocks[block_index] = received;
}

bool mbt_compare_hashes(struct mbt_file_handler *fh, size_t piece_index,
                        const char *data, size_t size)
{
    if (fh == NULL || piece_index >= fh->nb_pieces || data == NULL)
        return false;

    unsigned char computed_hash[SHA_DIGEST_LENGTH];
    SHA1((const unsigned char *)data, size, computed_hash);

    const unsigned char *expected_hash =
        (const unsigned char *)(fh->pieces_hash
                                + piece_index * SHA_DIGEST_LENGTH);
    return (memcmp(computed_hash, expected_hash, SHA_DIGEST_LENGTH) == 0);
}

enum mbt_piece_status mbt_piece_check(struct mbt_file_handler *fh,
                                      size_t piece_index)
{
    if (fh == NULL || piece_index >= fh->nb_pieces)
        return MBT_PIECE_INVALID;
    struct mbt_piece *p = &fh->pieces[piece_index];

    for (size_t i = 0; i < p->nb_blocks; i++)
    {
        if (!p->received_blocks[i])
            return MBT_PIECE_DOWNLOADING;
    }

    if (mbt_compare_hashes(fh, piece_index, p->data, p->size))
        return MBT_PIECE_VALID;
    else
        return MBT_PIECE_INVALID;
}

bool mbt_piece_write(struct mbt_file_handler *fh, size_t piece_index)
{
    if (fh == NULL || piece_index >= fh->nb_pieces)
        return false;
    struct mbt_piece *p = &fh->pieces[piece_index];

    size_t piece_length = mbt_torrent_get_piece_length(fh->torrent);
    size_t total_size = mbt_torrent_get_total_length(fh->torrent);

    size_t piece_offset = piece_index * piece_length;
    if (piece_offset + p->size > total_size)
        p->size = total_size - piece_offset;

    size_t remaining_size = p->size;
    size_t data_offset = 0;

    size_t nb_files = mbt_torrent_get_nb_files(fh->torrent);
    size_t cumulative_size = 0;

    for (size_t i = 0; i < nb_files && remaining_size > 0; i++)
    {
        struct mbt_file_info file_info;
        mbt_torrent_get_file_info(fh->torrent, i, &file_info);
        size_t file_start = cumulative_size;
        size_t file_end = cumulative_size + file_info.length;

        if (piece_offset < file_end && piece_offset + p->size > file_start)
        {
            size_t file_offset = 0;
            if (piece_offset > file_start)
                file_offset = piece_offset - file_start;

            size_t write_size = remaining_size;
            if (file_offset + write_size > file_info.length)
                write_size = file_info.length - file_offset;

            mbt_str_t path_str = mbt_str_from_cview(file_info.path);
            char *dir_path = strdup(path_str.data);
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
            mbt_str_dtor(&path_str);

            mbt_str_t file_path = mbt_str_from_cview(file_info.path);
            int fd = open(file_path.data, O_WRONLY | O_CREAT, 0644);
            mbt_str_dtor(&file_path);

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

        cumulative_size += file_info.length;
    }

    if (remaining_size > 0)
    {
        fprintf(stderr, "Could not write entire piece %zu.\n", piece_index);
        return false;
    }

    return true;
}
