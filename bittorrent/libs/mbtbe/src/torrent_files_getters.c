#include <mbt/be/torrent_impl.h>

struct mbt_cview mbt_torrent_file_path_get(const struct mbt_torrent_file *file,
                                           size_t idx)
{
    if (idx >= file->path_size)
    {
        return MBT_CVIEW(NULL, 0);
    }
    return MBT_CVIEW_OF(file->path[idx]);
}

size_t mbt_torrent_file_path_size(const struct mbt_torrent_file *file)
{
    return file->path_size;
}

size_t mbt_torrent_file_length(const struct mbt_torrent_file *file)
{
    return file->length;
}
