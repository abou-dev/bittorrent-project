#include "mbt/file/file_handler.h"

#include <mbt/be/torrent.h>
#include <mbt/be/torrent_getters.h>
#include <mbt/utils/str.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mbt/file/piece.h"

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

struct mbt_file_handler *mbt_file_handler_init(struct mbt_torrent *torrent)
{
    struct mbt_file_handler *fh = malloc(sizeof(struct mbt_file_handler));
    if (fh == NULL)
        return NULL;

    fh->torrent = torrent;

    fh->pieces_hash = mbt_torrent_pieces(fh->torrent);
    fh->nb_pieces = fh->pieces_hash.size / 20; // 20 octets par hash SHA1

    fh->pieces = malloc(sizeof(struct mbt_piece) * fh->nb_pieces);
    if (fh->pieces == NULL)
    {
        free(fh);
        return NULL;
    }
    memset(fh->pieces, 0, sizeof(struct mbt_piece) * fh->nb_pieces);

    size_t total_size = mbt_torrent_length(fh->torrent);
    size_t piece_length = mbt_torrent_piece_length(fh->torrent);

    for (size_t i = 0; i < fh->nb_pieces; i++)
    {
        struct mbt_piece *p = &fh->pieces[i];

        size_t piece_size = piece_length;
        if (i == fh->nb_pieces - 1)
        {
            size_t remaining = total_size - (piece_length * i);
            if (remaining < piece_length)
                piece_size = remaining;
        }
        p->size = piece_size;
        p->data = malloc(piece_size);
        if (p->data == NULL)
        {
            for (size_t j = 0; j < i; j++)
            {
                mbt_piece_dtor(&fh->pieces[j]);
            }
            free(fh->pieces);
            free(fh);
            return NULL;
        }
        memset(p->data, 0, piece_size);

        p->nb_blocks = (piece_size + MBT_BLOCK_SIZE - 1) / MBT_BLOCK_SIZE;
        p->received_blocks = malloc(sizeof(bool) * p->nb_blocks);
        if (p->received_blocks == NULL)
        {
            free(p->data);
            for (size_t j = 0; j < i; j++)
            {
                mbt_piece_dtor(&fh->pieces[j]);
            }
            free(fh->pieces);
            free(fh);
            return NULL;
        }
        memset(p->received_blocks, 0, sizeof(bool) * p->nb_blocks);
    }

    return fh;
}

void mbt_file_handler_free(struct mbt_file_handler *fh)
{
    if (fh == NULL)
        return;
    for (size_t i = 0; i < fh->nb_pieces; i++)
    {
        mbt_piece_dtor(&fh->pieces[i]);
    }
    free(fh->pieces);
    free(fh);
}

size_t mbt_file_handler_get_nb_pieces(struct mbt_file_handler *fh)
{
    return fh->nb_pieces;
}

struct mbt_cview mbt_file_handler_get_name(struct mbt_file_handler *fh)
{
    return mbt_torrent_name(fh->torrent);
}

size_t mbt_file_handler_get_nb_files(struct mbt_file_handler *fh)
{
    return mbt_torrent_files_size(fh->torrent);
}

size_t mbt_file_handler_get_total_size(struct mbt_file_handler *fh)
{
    return mbt_torrent_length(fh->torrent);
}

const char *mbt_file_handler_get_pieces_hash(struct mbt_file_handler *fh)
{
    return fh->pieces_hash.data;
}
