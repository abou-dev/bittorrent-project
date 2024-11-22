#include <mbt/be/torrent_impl.h>

struct mbt_cview mbt_torrent_announce(const struct mbt_torrent *torrent) {
    return MBT_CVIEW_OF(torrent->announce);
}

struct mbt_cview mbt_torrent_created_by(const struct mbt_torrent *torrent) {
    return MBT_CVIEW_OF(torrent->created_by);
}

size_t mbt_torrent_creation_date(const struct mbt_torrent *torrent) {
    return torrent->creation_date;
}

size_t mbt_torrent_piece_length(const struct mbt_torrent *torrent) {
    return torrent->piece_length;
}

struct mbt_cview mbt_torrent_name(const struct mbt_torrent *torrent) {
    return MBT_CVIEW_OF(torrent->name);
}

struct mbt_cview mbt_torrent_pieces(const struct mbt_torrent *torrent) {
    return MBT_CVIEW_OF(torrent->pieces);
}

size_t mbt_torrent_length(const struct mbt_torrent *torrent) {
    return torrent->length;
}

const struct mbt_torrent_file *mbt_torrent_files_get(const struct mbt_torrent *torrent, size_t idx) {
    if (idx >= torrent->files_size) {
        return NULL;
    }
    return torrent->files[idx];
}

size_t mbt_torrent_files_size(const struct mbt_torrent *torrent) {
    return torrent->files_size;
}

bool mbt_torrent_is_dir(const struct mbt_torrent *torrent) {
    return torrent->files_size > 1;
}

const struct mbt_be_node *mbt_torrent_node(const struct mbt_torrent *torrent) {
    return torrent->node;
}


	

