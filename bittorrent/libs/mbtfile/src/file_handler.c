#include "mbt/file/file_handler.h"

#include <errno.h>
#include <fcntl.h>
#include <mbt/be/torrent.h>
#include <mbt/utils/str.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mbt/file/piece.h"

struct mbt_file_handler
{
    struct mbt_torrent *torrent;
    struct mbt_piece *pieces;
    size_t nb_pieces;
    const char *pieces_hash;
};

struct mbt_file_handler *mbt_file_handler_init(struct mbt_torrent *torrent)
{
    if (torrent == NULL)
        return NULL;

    struct mbt_file_handler *fh = malloc(sizeof(struct mbt_file_handler));
    if (fh == NULL)
        return NULL;

    fh->torrent = torrent;

    fh->nb_pieces = mbt_torrent_get_nb_pieces(torrent);

    fh->pieces = malloc(sizeof(struct mbt_piece) * fh->nb_pieces);
    if (fh->pieces == NULL)
    {
        free(fh);
        return NULL;
    }
    memset(fh->pieces, 0, sizeof(struct mbt_piece) * fh->nb_pieces);

    fh->pieces_hash = mbt_torrent_get_pieces_hash(torrent);

    size_t total_size = mbt_torrent_get_total_length(torrent);
    size_t piece_length = mbt_torrent_get_piece_length(torrent);

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
    if (fh == NULL)
        return 0;
    return fh->nb_pieces;
}

struct mbt_cview mbt_file_handler_get_name(struct mbt_file_handler *fh)
{
    if (fh == NULL)
        return (struct mbt_cview){ NULL, 0 };
    return mbt_torrent_get_name(fh->torrent);
}

size_t mbt_file_handler_get_nb_files(struct mbt_file_handler *fh)
{
    if (fh == NULL)
        return 0;
    return mbt_torrent_get_nb_files(fh->torrent);
}

size_t mbt_file_handler_get_total_size(struct mbt_file_handler *fh)
{
    if (fh == NULL)
        return 0;
    return mbt_torrent_get_total_length(fh->torrent);
}

const char *mbt_file_handler_get_pieces_hash(struct mbt_file_handler *fh)
{
    if (fh == NULL)
        return NULL;
    return fh->pieces_hash;
}
