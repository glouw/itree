#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <utility>

// ============================================================
// 1. Shared virtual root
// ============================================================

struct KernelObject {
    virtual ~KernelObject() = default;

    virtual const char* type_name() const = 0;
    virtual void shutdown() = 0;
};

// ============================================================
// 2. Independent interfaces
// ============================================================

struct IFileSystem : virtual public KernelObject {
    virtual bool mount(const std::string&) = 0;
    virtual void unmount() = 0;

    virtual bool exists(const std::string&) const = 0;
};

struct IBlockDevice : virtual public KernelObject {
    virtual bool read_blocks(
        std::uint64_t lba,
        std::uint32_t count,
        void* buffer
    ) = 0;

    virtual bool write_blocks(
        std::uint64_t lba,
        std::uint32_t count,
        const void* buffer
    ) = 0;
};

struct IJournal : virtual public KernelObject {
    virtual void begin_transaction() = 0;
    virtual void commit_transaction() = 0;
    virtual void rollback_transaction() = 0;
};

struct IMountable : virtual public KernelObject {
    virtual std::string mount_point() const = 0;
};

// ============================================================
// 3. Protected implementation inheritance
// ============================================================

namespace Test
{
class PathResolver : protected virtual KernelObject {
protected:
    std::string normalize_path(const std::string& path) const {
        // TODO: canonicalize path
        return path;
    }

    bool resolve_components(const std::string&) const {
        // TODO: resolve path components
        return false;
    }

public:
    virtual ~PathResolver() = default;

    virtual void invalidate_cache() {
        // TODO: invalidate path cache
    }
};
}

using namespace Test;

class MetadataCache : protected virtual KernelObject {
protected:
    struct CacheEntry {
        std::uint64_t inode;
        bool dirty;
    };

    std::unordered_map<std::string, CacheEntry> cache_;

    void flush_metadata() {
        // TODO: flush dirty metadata
    }

public:
    virtual ~MetadataCache() = default;

    virtual void invalidate_cache() {
        cache_.clear();
    }
};

// ============================================================
// 4. A diamond with two implementations of invalidate_cache
// ============================================================

class FilesystemCore
    : public IFileSystem
    , public IBlockDevice
    , public IJournal
    , public IMountable
    , protected PathResolver
    , protected MetadataCache
{
public:
    // Resolve the ambiguity between two inherited implementations.
    void invalidate_cache() override {
        PathResolver::invalidate_cache();
        MetadataCache::invalidate_cache();
    }

    // KernelObject is a virtual base, so there is one shared
    // KernelObject subobject in the complete object.
    const char* type_name() const override {
        return "FilesystemCore";
    }

    void shutdown() override {
        unmount();
    }

    bool mount(const std::string& device) override {
        // TODO: mount lifecycle
        return false;
    }

    void unmount() override {
        // TODO: unmount lifecycle
    }

    bool exists(const std::string& path) const override {
        return resolve_components(normalize_path(path));
    }

    bool read_blocks(
        std::uint64_t,
        std::uint32_t,
        void*
    ) override {
        return false;
    }

    bool write_blocks(
        std::uint64_t,
        std::uint32_t,
        const void*
    ) override {
        return false;
    }

    void begin_transaction() override {}
    void commit_transaction() override {}
    void rollback_transaction() override {}

    std::string mount_point() const override {
        return "/";
    }

protected:
    // Derived filesystems can customize these.
    virtual void load_superblock() {}
    virtual void write_superblock() {}
    virtual void recover_journal() {}
};

// ============================================================
// 5. Private inheritance: composition-like implementation reuse
// ============================================================

class AllocationBitmap {
protected:
    bool allocate_block() {
        // TODO
        return false;
    }

    void release_block(std::uint64_t) {
        // TODO
    }

public:
    virtual ~AllocationBitmap() = default;

    virtual std::uint64_t free_blocks() const {
        return 0;
    }
};

class InodeTable {
protected:
    std::uint64_t find_inode(const std::string&) const {
        // TODO
        return 0;
    }

    bool persist_inode(std::uint64_t) {
        // TODO
        return false;
    }

public:
    virtual ~InodeTable() = default;

    virtual std::uint64_t inode_count() const {
        return 0;
    }
};

// ============================================================
// 6. Concrete filesystem with private inheritance
// ============================================================

class ExtFilesystem
    : public FilesystemCore
    , private AllocationBitmap
    , private InodeTable
{
public:
    using FilesystemCore::mount;
    using FilesystemCore::unmount;
    using FilesystemCore::exists;

    // Publicly expose selected capabilities from private bases.
    using AllocationBitmap::free_blocks;
    using InodeTable::inode_count;

    const char* type_name() const override {
        return "ExtFilesystem";
    }

    bool mount(const std::string& device) override {
        if (!FilesystemCore::mount(device)) {
            return false;
        }

        load_superblock();
        recover_journal();
        return true;
    }

    void unmount() override {
        write_superblock();
        FilesystemCore::unmount();
    }

    void shutdown() override {
        unmount();
    }

    // New filesystem-specific operations.
    bool create_file(const std::string& path) {
        const auto inode = find_inode(path);

        if (inode != 0) {
            return false;
        }

        if (!allocate_block()) {
            return false;
        }

        // TODO: allocate inode and persist directory entry
        return true;
    }

    bool delete_file(const std::string& path) {
        const auto inode = find_inode(path);

        if (inode == 0) {
            return false;
        }

        // TODO: remove directory entry and free resources
        return persist_inode(inode);
    }
};

// ============================================================
// 7. Another interface layered on the concrete filesystem
// ============================================================

struct ISnapshotSupport : virtual public KernelObject {
    virtual bool create_snapshot(const std::string&) = 0;
};

class SnapshotFilesystem
    : public ExtFilesystem
    , public ISnapshotSupport
{
public:
    const char* type_name() const override {
        return "SnapshotFilesystem";
    }

    bool create_snapshot(const std::string& name) override {
        // TODO: snapshot metadata and block mappings
        return !name.empty();
    }
};

// ============================================================
// 8. Runtime polymorphism
// ============================================================

void inspect(KernelObject& object) {
    // Dispatches to the most-derived override.
    object.type_name();

    // Dispatches to the most-derived shutdown override.
    // object.shutdown();
}

void operate(IFileSystem& fs) {
    fs.mount("device0");
    fs.exists("/etc/config");
    fs.unmount();
}

void transact(IJournal& journal) {
    journal.begin_transaction();
    // TODO: mutate metadata
    journal.commit_transaction();
}

int main() {
    SnapshotFilesystem fs;

    KernelObject& root = fs;
    IFileSystem& filesystem = fs;
    IBlockDevice& device = fs;
    IJournal& journal = fs;
    ISnapshotSupport& snapshots = fs;

    inspect(root);
    operate(filesystem);
    transact(journal);

    device.read_blocks(0, 1, nullptr);
    snapshots.create_snapshot("before-upgrade");

    fs.create_file("/example");
    fs.free_blocks();
    fs.inode_count();

    return 0;
}
