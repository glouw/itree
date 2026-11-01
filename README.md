## itree

Explore inheritance hierarchies locally in your terminal:

```
 class K::AllocationBitmap main.cc:185
 ├── protected bool allocate_block();
 ├── protected void release_block(uint64_t);
 ├── public virtual ~AllocationBitmap();
 ├── public virtual uint64_t free_blocks();
 └── extends with private K::ExtFilesystem main.cc:228
     ├── public virtual const char * type_name() override;
     ├── public virtual bool mount(const std::string & device) override;
     ├── public virtual void unmount() override;
     ├── public virtual void shutdown() override;
     ├── public bool create_file(const std::string & path);
     ├── public bool delete_file(const std::string & path);
     └── extends with public K::SnapshotFilesystem main.cc:301
         ├── public virtual const char * type_name() multiple override;
         └── public virtual bool create_snapshot(const std::string & name) override;

 struct K::KernelObject main.cc:15
 ├── public virtual ~KernelObject();
 ├── public virtual const char * type_name() = 0;
 ├── public virtual void shutdown() = 0;
 └── extends with public virtual K::IFileSystem<int> main.cc:27
     ├── public virtual bool mount(const std::string &) = 0;
     ├── public virtual void unmount() = 0;
     ├── public virtual bool exists(const std::string &) = 0;
     └── extends with public K::FilesystemCore main.cc:112
         └── public virtual void invalidate_cache() multiple override;
```

## Usage

```
make
cd to/your/project
.~/itree/itree
```

A `compile_commands.json` will be required. Generate one with either with `make` and `bear` or CMake's `EXPORT_COMPILE_COMMANDS`.
Be sure to somewhere specify C++'s standard library include location with `-isystem` flag.

### Deps

* GNU readline
* LLVM libtooling
