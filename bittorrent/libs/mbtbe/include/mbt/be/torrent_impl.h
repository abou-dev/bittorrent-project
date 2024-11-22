#ifndef MBT_BE_TORRENT_IMPL_H
#define MBT_BE_TORRENT_IMPL_H

#include <mbt/be/torrent.h>
#include <mbt/be/bencode.h>
#include <mbt/utils/str.h>
#include <stdbool.h>
#include <stddef.h>

struct mbt_torrent_file {
    struct mbt_str *path;  
    size_t path_size;      
    size_t length;      
};

struct mbt_torrent {
    struct mbt_str announce;
    struct mbt_str created_by;
    size_t creation_date;
    size_t piece_length;
    struct mbt_str name;
    struct mbt_str pieces;
    size_t length;
    struct mbt_torrent_file **files;
    size_t files_size;
    struct mbt_be_node *node;
};

#endif // MBT_BE_TORRENT_IMPL_H



	

