// libs/mbtfile/tests/test_mbtfile.c

#include <criterion/criterion.h>
#include "mbt/file/file_handler.h"
#include "mbt/be/torrent.h"
#include "mbt/utils/str.h"
#include <string.h>

// Mock structures and functions for mbt_torrent
struct mbt_file_info {
    struct mbt_cview path;
    size_t length;
};

struct mbt_info {
    size_t nb_pieces;
    const char *pieces; // SHA1 hashes concatenés
    struct mbt_cview name;
    size_t nb_files;
    struct mbt_file_info *files;
    size_t total_length;
    size_t piece_length;
};

struct mbt_torrent {
    struct mbt_info info;
};

// Mock functions for mbtbe (BitTorrent Backend)
size_t mbt_torrent_get_nb_pieces(struct mbt_torrent *torrent) {
    return torrent->info.nb_pieces;
}

const char *mbt_torrent_get_pieces_hash(struct mbt_torrent *torrent) {
    return torrent->info.pieces;
}

struct mbt_cview mbt_torrent_get_name(struct mbt_torrent *torrent) {
    return torrent->info.name;
}

size_t mbt_torrent_get_nb_files(struct mbt_torrent *torrent) {
    return torrent->info.nb_files;
}

size_t mbt_torrent_get_total_length(struct mbt_torrent *torrent) {
    return torrent->info.total_length;
}

size_t mbt_torrent_get_piece_length(struct mbt_torrent *torrent) {
    return torrent->info.piece_length;
}

bool mbt_torrent_get_file_info(struct mbt_torrent *torrent, size_t index, struct mbt_file_info *out) {
    if (index >= torrent->info.nb_files)
        return false;
    *out = torrent->info.files[index];
    return true;
}

size_t mbt_torrent_get_piece_size(struct mbt_torrent *torrent, size_t piece_index) {
    size_t piece_length = mbt_torrent_get_piece_length(torrent);
    if (piece_index == mbt_torrent_get_nb_pieces(torrent) - 1) {
        size_t remainder = mbt_torrent_get_total_length(torrent) % piece_length;
        return (remainder == 0) ? piece_length : remainder;
    }
    return piece_length;
}

// Helper function to create a mock torrent
struct mbt_torrent *create_mock_torrent() {
    struct mbt_torrent *torrent = malloc(sizeof(struct mbt_torrent));
    if (!torrent)
        return NULL;

    torrent->info.nb_pieces = 2;
    // Example SHA1 hashes (20 bytes each)
    torrent->info.pieces = "01234567890123456789ABCDEFGHIJKLMNOPQRSTUVWX";
    torrent->info.name = (struct mbt_cview){ "test_torrent", strlen("test_torrent") };
    torrent->info.nb_files = 2;

    torrent->info.files = malloc(sizeof(struct mbt_file_info) * torrent->info.nb_files);
    if (!torrent->info.files) {
        free(torrent);
        return NULL;
    }

    // File 1
    torrent->info.files[0].path = (struct mbt_cview){ "file1.txt", strlen("file1.txt") };
    torrent->info.files[0].length = 10000;

    // File 2
    torrent->info.files[1].path = (struct mbt_cview){ "file2.txt", strlen("file2.txt") };
    torrent->info.files[1].length = 20000;

    torrent->info.total_length = 30000;
    torrent->info.piece_length = 16384;

    return torrent;
}

// Test initialization and getters
Test(mbtfile, init_and_getters) {
    struct mbt_torrent *torrent = create_mock_torrent();
    cr_assert_not_null(torrent, "Failed to create mock torrent");

    struct mbt_file_handler *fh = mbt_file_handler_init(torrent);
    cr_assert_not_null(fh, "Failed to initialize mbt_file_handler");

    cr_assert_eq(mbt_file_handler_get_nb_pieces(fh), 2, "Number of pieces should be 2");
    cr_assert_str_eq(mbt_file_handler_get_name(fh).data, "test_torrent", "Torrent name mismatch");
    cr_assert_eq(mbt_file_handler_get_nb_files(fh), 2, "Number of files should be 2");
    cr_assert_eq(mbt_file_handler_get_total_size(fh), 30000, "Total size should be 30000");
    cr_assert_str_eq(mbt_file_handler_get_pieces_hash(fh), "01234567890123456789ABCDEFGHIJKLMNOPQRSTUVWX", "Pieces hash mismatch");

    mbt_file_handler_free(fh);

    // Clean up
    free(torrent->info.files);
    free(torrent);
}

// Test setting and getting piece data
Test(mbtfile, set_and_get_piece_data) {
    struct mbt_torrent *torrent = create_mock_torrent();
    cr_assert_not_null(torrent, "Failed to create mock torrent");

    struct mbt_file_handler *fh = mbt_file_handler_init(torrent);
    cr_assert_not_null(fh, "Failed to initialize mbt_file_handler");

    const char *data = "This is a test piece data.";
    size_t piece_index = 0;
    size_t data_size = strlen(data) + 1;

    mbt_piece_set_data(fh, piece_index, data, data_size);
    const char *retrieved_data = mbt_piece_get_data(fh, piece_index);
    cr_assert_str_eq(retrieved_data, data, "Piece data mismatch");

    mbt_file_handler_free(fh);

    // Clean up
    free(torrent->info.files);
    free(torrent);
}

// Test block reception
Test(mbtfile, block_reception) {
    struct mbt_torrent *torrent = create_mock_torrent();
    cr_assert_not_null(torrent, "Failed to create mock torrent");

    struct mbt_file_handler *fh = mbt_file_handler_init(torrent);
    cr_assert_not_null(fh, "Failed to initialize mbt_file_handler");

    size_t piece_index = 0;
    size_t block_index = 1;

    cr_assert_not(mbt_piece_block_is_received(fh, piece_index, block_index), "Block should not be received initially");

    mbt_piece_block_set_received(fh, piece_index, block_index, true);
    cr_assert(mbt_piece_block_is_received(fh, piece_index, block_index), "Block should be marked as received");

    mbt_file_handler_free(fh);

    // Clean up
    free(torrent->info.files);
    free(torrent);
}

// Test hash comparison (assuming the data matches the mock hash)
Test(mbtfile, compare_hashes) {
    struct mbt_torrent *torrent = create_mock_torrent();
    cr_assert_not_null(torrent, "Failed to create mock torrent");

    struct mbt_file_handler *fh = mbt_file_handler_init(torrent);
    cr_assert_not_null(fh, "Failed to initialize mbt_file_handler");

    size_t piece_index = 0;
    const char *data = "This is a test piece data."; // Note: This won't match the mock hash

    bool result = mbt_compare_hashes(fh, piece_index, data);
    cr_assert_not(result, "Hash comparison should fail for mismatched data");

    // For a successful hash comparison, you need to provide data that matches the mock hash
    // Since our mock hash is arbitrary, we'll skip the successful case here

    mbt_file_handler_free(fh);

    // Clean up
    free(torrent->info.files);
    free(torrent);
}

