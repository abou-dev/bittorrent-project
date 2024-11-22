#include "mbt/file/piece.h"
#include "mbt/file/file_handler.h"

#include <mbt/utils/str.h>
#include <mbt/be/torrent.h>

#include <openssl/sha.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

struct mbt_piece {
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
                        const char *received_data)
{
    if (fh == NULL || piece_index >= fh->nb_pieces || received_data == NULL)
        return false;
    struct mbt_piece *p = &fh->pieces[piece_index];

    unsigned char computed_hash[SHA_DIGEST_LENGTH];
    SHA1((const unsigned char *)received_data, p->size, computed_hash);

    const unsigned char *expected_hash = (const unsigned char *)(fh->pieces_hash + piece_index * SHA_DIGEST_LENGTH);
    return (memcmp(computed_hash, expected_hash, SHA_DIGEST_LENGTH) == 0);
}
enum mbt_piece_status mbt_piece_check(struct mbt_file_handler *fh,
                                      size_t piece_index)
{
    if (fh == NULL || piece_index >= fh->nb_pieces)
        return MBT_PIECE_INVALID;
    struct mbt_piece *p = &fh->pieces[piece_index];

    // Vérifier si tous les blocs ont été reçus
    for (size_t i = 0; i < p->nb_blocks; i++) {
        if (!p->received_blocks[i])
            return MBT_PIECE_DOWNLOADING;
    }

    // Tous les blocs reçus, vérifier le hash
    if (mbt_compare_hashes(fh, piece_index, p->data))
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
    size_t piece_offset = piece_index * piece_length;

    size_t cumulative_size = 0;
    size_t nb_files = mbt_torrent_get_nb_files(fh->torrent);
    for (size_t i = 0; i < nb_files; i++) {
        struct mbt_file_info file_info;
        mbt_torrent_get_file_info(fh->torrent, i, &file_info);
        if (piece_offset < cumulative_size + file_info.length) {
            // La pièce appartient à ce fichier
            size_t file_offset = piece_offset - cumulative_size;

            // Déterminer la taille à écrire
            size_t write_size = p->size;
            if (file_offset + write_size > file_info.length)
                write_size = file_info.length - file_offset;

            // Ouvrir le fichier
            int fd = open(file_info.path.data, O_WRONLY | O_CREAT, 0644);
            if (fd == -1) {
                perror("open failed");
                return false;
            }

            // Se positionner au bon offset
            if (lseek(fd, file_offset, SEEK_SET) == (off_t)-1) {
                perror("lseek failed");
                close(fd);
                return false;
            }

            // Écrire les données
            ssize_t written = write(fd, p->data, write_size);
            if (written != (ssize_t)write_size) {
                perror("write failed");
                close(fd);
                return false;
            }

            close(fd);
            return true;
        }
        cumulative_size += file_info.length;
    }

    // La pièce n'appartient à aucun fichier (ne devrait pas arriver)
    fprintf(stderr, "Piece index %zu does not belong to any file.\n", piece_index);
    return false;
}

