#include <mbt/be/bencode.h>
#include <mbt/be/torrent_impl.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static void *xmalloc(size_t size)
{
    void *ptr = malloc(size);
    if (!ptr)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

static void *xcalloc(size_t count, size_t size)
{
    void *ptr = calloc(count, size);
    if (!ptr)
    {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

static struct mbt_torrent_file *mbt_torrent_file_init(struct mbt_be_node *node)
{
    struct mbt_torrent_file *file = xmalloc(sizeof(*file));
    if (!file)
    {
        return NULL;
    }

    struct mbt_be_node **path_list = node->v.dict[1]->val->v.list;
    size_t path_size = 0;
    for (; path_list[path_size]; ++path_size)
    {
    }

    file->path = xmalloc(path_size * sizeof(struct mbt_str));
    file->path_size = path_size;

    for (size_t i = 0; i < path_size; ++i)
    {
        mbt_str_ctor(&file->path[i], path_list[i]->v.str.size);
        mbt_str_pushcv(&file->path[i], MBT_CVIEW_OF(path_list[i]->v.str));
    }

    file->length = node->v.dict[0]->val->v.nb;

    return file;
}

struct mbt_torrent *mbt_torrent_init(void)
{
    struct mbt_torrent *torrent = xcalloc(1, sizeof(*torrent));
    if (!torrent)
    {
        return NULL;
    }
    mbt_str_ctor(&torrent->announce, 0);
    mbt_str_ctor(&torrent->created_by, 0);
    mbt_str_ctor(&torrent->name, 0);
    mbt_str_ctor(&torrent->pieces, 0);
    return torrent;
}

void mbt_torrent_free(struct mbt_torrent *torrent)
{
    if (!torrent)
    {
        return;
    }
    mbt_str_dtor(&torrent->announce);
    mbt_str_dtor(&torrent->created_by);
    mbt_str_dtor(&torrent->name);
    mbt_str_dtor(&torrent->pieces);

    if (torrent->files)
    {
        for (size_t i = 0; i < torrent->files_size; ++i)
        {
            for (size_t j = 0; j < torrent->files[i]->path_size; ++j)
            {
                mbt_str_dtor(&torrent->files[i]->path[j]);
            }
            free(torrent->files[i]->path);
            free(torrent->files[i]);
        }
        free(torrent->files);
    }

    if (torrent->node)
    {
        mbt_be_free(torrent->node);
    }

    free(torrent);
}

bool mbt_be_parse_torrent_file(const char *path, struct mbt_torrent *torrent)
{
    FILE *file = fopen(path, "rb");
    if (!file)
    {
        return false;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char *buffer = xmalloc(file_size);
    if (!buffer)
    {
        fclose(file);
        return false;
    }

    fread(buffer, 1, file_size, file);
    fclose(file);

    struct mbt_cview view = MBT_CVIEW(buffer, file_size);
    struct mbt_be_node *root = mbt_be_decode(&view);
    if (!root)
    {
        free(buffer);
        return false;
    }

    torrent->node = root;

    mbt_str_ctor(&torrent->announce, root->v.dict[0]->val->v.str.size);
    mbt_str_pushcv(&torrent->announce,
                   MBT_CVIEW_OF(root->v.dict[0]->val->v.str));

    mbt_str_ctor(&torrent->created_by, root->v.dict[1]->val->v.str.size);
    mbt_str_pushcv(&torrent->created_by,
                   MBT_CVIEW_OF(root->v.dict[1]->val->v.str));

    torrent->creation_date = root->v.dict[2]->val->v.nb;

    torrent->piece_length = root->v.dict[3]->val->v.nb;

    mbt_str_ctor(&torrent->name, root->v.dict[4]->val->v.str.size);
    mbt_str_pushcv(&torrent->name, MBT_CVIEW_OF(root->v.dict[4]->val->v.str));

    mbt_str_ctor(&torrent->pieces, root->v.dict[5]->val->v.str.size);
    mbt_str_pushcv(&torrent->pieces, MBT_CVIEW_OF(root->v.dict[5]->val->v.str));

    torrent->length = root->v.dict[6]->val->v.nb;

    struct mbt_be_node **file_list = root->v.dict[7]->val->v.list;
    size_t file_count = 0;
    for (; file_list[file_count]; ++file_count)
    {
    }

    torrent->files = xmalloc(file_count * sizeof(struct mbt_torrent_file *));
    torrent->files_size = file_count;

    for (size_t i = 0; i < file_count; ++i)
    {
        torrent->files[i] = mbt_torrent_file_init(file_list[i]);
    }

    free(buffer);
    return true;
}

static void compute_sha1(const char *data, size_t size, unsigned char *hash)
{
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, data, size);
    SHA1_Final(hash, &ctx);
}

static struct mbt_str compute_pieces(const char *path, size_t piece_length)
{
    FILE *file = fopen(path, "rb");
    if (!file)
    {
        struct mbt_str empty_str;
        mbt_str_ctor(&empty_str, 0);
        return empty_str;
    }

    struct mbt_str pieces;
    mbt_str_ctor(&pieces, 0);

    char *buffer = xmalloc(piece_length);
    size_t bytes_read;
    unsigned char hash[SHA_DIGEST_LENGTH];

    while ((bytes_read = fread(buffer, 1, piece_length, file)) > 0)
    {
        compute_sha1(buffer, bytes_read, hash);
        mbt_str_pushcv(&pieces, MBT_CVIEW((char *)hash, SHA_DIGEST_LENGTH));
    }

    fclose(file);
    free(buffer);

    return pieces;
}

bool mbt_be_make_torrent_file(const char *path)
{
    struct stat path_stat;
    if (stat(path, &path_stat) == -1)
    {
        perror("stat");
        return false;
    }

    struct mbt_be_pair **torrent_dict = xmalloc(8 * sizeof(*torrent_dict));
    torrent_dict[7] = NULL;

    torrent_dict[0] = mbt_be_pair_init(
        MBT_CVIEW_LIT("announce"),
        mbt_be_str_init(MBT_CVIEW_LIT("http://localhost:6969/announce")));

    torrent_dict[1] = mbt_be_pair_init(
        MBT_CVIEW_LIT("created by"),
        mbt_be_str_init(MBT_CVIEW_LIT("My BitTorrent Client")));

    torrent_dict[2] = mbt_be_pair_init(MBT_CVIEW_LIT("creation date"),
                                       mbt_be_num_init(time(NULL)));

    struct mbt_be_pair **info_dict = xmalloc(6 * sizeof(*info_dict));
    info_dict[5] = NULL;

    const size_t piece_length = 256 * 1024; // 256 KB
    info_dict[0] = mbt_be_pair_init(MBT_CVIEW_LIT("piece length"),
                                    mbt_be_num_init(piece_length));

    struct mbt_str pieces = compute_pieces(path, piece_length);
    if (!pieces.data)
    {
        free(torrent_dict);
        free(info_dict);
        return false;
    }
    info_dict[1] = mbt_be_pair_init(MBT_CVIEW_LIT("pieces"),
                                    mbt_be_str_init(MBT_CVIEW_OF(pieces)));

    info_dict[2] = mbt_be_pair_init(MBT_CVIEW_LIT("name"),
                                    mbt_be_str_init(MBT_CVIEW_LIT(path)));

    info_dict[3] = mbt_be_pair_init(MBT_CVIEW_LIT("length"),
                                    mbt_be_num_init(path_stat.st_size));

    struct mbt_be_node *info_node = mbt_be_dict_init(info_dict);
    torrent_dict[6] = mbt_be_pair_init(MBT_CVIEW_LIT("info"), info_node);

    struct mbt_be_node *torrent_node = mbt_be_dict_init(torrent_dict);
    struct mbt_str encoded = mbt_be_encode(torrent_node);

    char torrent_file[512];
    snprintf(torrent_file, sizeof(torrent_file), "%s.torrent", path);
    FILE *output = fopen(torrent_file, "wb");
    if (!output)
    {
        mbt_str_dtor(&encoded);
        mbt_be_free(torrent_node);
        return false;
    }

    fwrite(encoded.data, 1, encoded.size, output);
    fclose(output);

    mbt_str_dtor(&encoded);
    mbt_be_free(torrent_node);
    return true;
}
