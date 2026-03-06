const OPFS_META_ENTRY_SIZE = 8; // index + offset

export interface Store {
    keys: () => Promise<number[]>;
    put: (key: number, data: Uint8Array) => Promise<void>;
    getSync: (key: number) => Uint8Array | null;
    getAsync: (key: number) => Promise<Uint8Array | null>;
}

export class NoStore implements Store {
    store: Map<number, Uint8Array> = new Map();
    put(key: number, data: Uint8Array): Promise<void> {
        this.store.set(key, data);
        return Promise.resolve();
    }
    getSync(key: number): Uint8Array | null {
        return this.store.get(key) ?? null;
    }
    getAsync(_: number): Promise<Uint8Array | null> {
        return Promise.resolve(null);
    }
    keys(): Promise<number[]> {
        return Promise.resolve(Array.from(this.store.keys()));
    }
}

function urlToDirectory(owner: string): string {
    return owner
        .replace(/^https?:\/\//, "")
        .replace(/[^a-zA-Z0-9._-]/g, "_")
        .substring(0, 200);
}

interface SyncAccessHandle {
    read(buffer: Uint8Array, opts: { at: number }): number;
    write(data: Uint8Array, opts: { at: number }): number;
    truncate(size: number): void;
    flush(): void;
    getSize(): number;
    close(): void;
}

interface AsyncFileHandle {
    getFile(): Promise<File>;
    createWritable(opts?: { keepExistingData?: boolean }): Promise<{
        seek(offset: number): Promise<void>;
        write(data: Uint8Array): Promise<void>;
        close(): Promise<void>;
    }>;
}

type Handle = SyncAccessHandle | AsyncFileHandle;

export class OpfsStore implements Store {
    private dir: FileSystemDirectoryHandle;
    private index: Map<number, number> = new Map();
    private syncMode: boolean = false;
    private blockHandle: Handle | null = null;
    private metaHandle: Handle | null = null;
    private blockSize: number = 0;
    private metaSize: number = 0;
    private blockLength: number;
    private metaEntry = new Uint8Array(OPFS_META_ENTRY_SIZE);

    private constructor(dir: FileSystemDirectoryHandle, blockLength: number) {
        this.dir = dir;
        this.blockLength = blockLength;
    }

    static async create(url: string, blockLength: number): Promise<OpfsStore> {
        const root = await navigator.storage.getDirectory();
        const jsdosRoot = await root.getDirectoryHandle("jsdos", { create: true });
        const cachesRoot = await jsdosRoot.getDirectoryHandle("caches", { create: true });
        const sockdriveRoot = await cachesRoot.getDirectoryHandle("sockdrive", { create: true });
        const dir = await sockdriveRoot.getDirectoryHandle(urlToDirectory(url), { create: true });
        const store = new OpfsStore(dir, blockLength);
        await store.init();
        return store;
    }

    private async init(): Promise<void> {
        let metaData: Uint8Array | null = null;
        try {
            const file = await (await this.dir.getFileHandle("_meta")).getFile();
            if (file.size >= OPFS_META_ENTRY_SIZE) {
                metaData = new Uint8Array(await file.arrayBuffer());
            }
        } catch {/* meta doesn't exist yet */}

        if (metaData && this.loadIndex(metaData)) {
            this.metaSize = metaData.byteLength;
        } else {
            this.index.clear();
            this.metaSize = 0;
            try {
                await this.dir.removeEntry("_block");
            } catch { }
            try {
                await this.dir.removeEntry("_meta");
            } catch { }
        }

        const blockFH = await this.dir.getFileHandle("_block", { create: true });
        const metaFH = await this.dir.getFileHandle("_meta", { create: true });

        // Default: sync mode via createSyncAccessHandle (workers)
        try {
            const blockSH = await (blockFH as unknown as {
                createSyncAccessHandle(): Promise<SyncAccessHandle>;
            }).createSyncAccessHandle();
            this.blockHandle = blockSH;
            const metaSH = await (metaFH as unknown as {
                createSyncAccessHandle(): Promise<SyncAccessHandle>;
            }).createSyncAccessHandle();
            this.metaHandle = metaSH;
            this.blockSize = blockSH.getSize();
            this.metaSize = metaSH.getSize();
            this.syncMode = true;
        } catch {
            try {
                (this.blockHandle as SyncAccessHandle)?.close();
            } catch { }
            try {
                (this.metaHandle as SyncAccessHandle)?.close();
            } catch { }
            this.blockHandle = null;
            this.metaHandle = null;

            // Fallback: async mode (main thread where sync access is unavailable)
            console.warn("OPFS sync mode unavailable, using async fallback");
            this.blockHandle = blockFH as unknown as AsyncFileHandle;
            this.metaHandle = metaFH as unknown as AsyncFileHandle;
            const file = await (this.blockHandle as AsyncFileHandle).getFile();
            this.blockSize = file.size;
        }

        // Validate: block file size must match the number of meta entries
        const expectedBlockSize = (this.metaSize / OPFS_META_ENTRY_SIZE) * this.blockLength;
        if (this.blockSize !== expectedBlockSize) {
            console.warn("OPFS block/meta size mismatch: block", this.blockSize,
                "expected", expectedBlockSize, "- resetting");
            this.index.clear();
            this.blockSize = 0;
            this.metaSize = 0;
            if (this.syncMode) {
                (this.blockHandle as SyncAccessHandle).truncate(0);
                (this.metaHandle as SyncAccessHandle).truncate(0);
            } else {
                const bw = await (this.blockHandle as AsyncFileHandle).createWritable();
                await bw.close();
                const mw = await (this.metaHandle as AsyncFileHandle).createWritable();
                await mw.close();
            }
        }
    }

    private loadIndex(data: Uint8Array): boolean {
        let pos = 0;
        while (pos + OPFS_META_ENTRY_SIZE <= data.byteLength) {
            const key = readUint32(data, pos);
            const offset = readUint32(data, pos + 4);
            this.index.set(key, offset);
            pos += OPFS_META_ENTRY_SIZE;
        }
        if (pos !== data.byteLength) {
            console.warn("OPFS meta file corrupted: expected", pos, "bytes, got", data.byteLength);
            return false;
        }
        return true;
    }

    async keys(): Promise<number[]> {
        return Array.from(this.index.keys());
    }

    async put(key: number, data: Uint8Array): Promise<void> {
        const offset = this.blockSize;
        writeUint32(this.metaEntry, key, 0);
        writeUint32(this.metaEntry, offset, 4);

        if (this.syncMode) {
            const block = this.blockHandle as SyncAccessHandle;
            const meta = this.metaHandle as SyncAccessHandle;
            block.write(data, { at: offset });
            meta.write(this.metaEntry, { at: this.metaSize });
            block.flush();
            meta.flush();
        } else {
            const blockW = await (this.blockHandle as AsyncFileHandle).createWritable({ keepExistingData: true });
            await blockW.seek(offset);
            await blockW.write(data);
            await blockW.close();
            const metaW = await (this.metaHandle as AsyncFileHandle).createWritable({ keepExistingData: true });
            await metaW.seek(this.metaSize);
            await metaW.write(this.metaEntry);
            await metaW.close();
        }

        this.blockSize += this.blockLength;
        this.metaSize += OPFS_META_ENTRY_SIZE;
        this.index.set(key, offset);
    }

    getSync(key: number): Uint8Array | null {
        if (!this.syncMode) {
            return null;
        }
        const offset = this.index.get(key);
        if (offset === undefined) {
            return null;
        }

        const buffer = new Uint8Array(this.blockLength);
        (this.blockHandle as SyncAccessHandle).read(buffer, { at: offset });
        return buffer;
    }

    async getAsync(key: number): Promise<Uint8Array | null> {
        if (this.syncMode) {
            return null;
        }

        const offset = this.index.get(key);
        if (offset === undefined) {
            return null;
        }

        const file = await (this.blockHandle as AsyncFileHandle).getFile();
        return new Uint8Array(await file.slice(offset, offset + this.blockLength).arrayBuffer());
    }
}

export async function getStore(url: string, blockLength: number): Promise<Store> {
    try {
        if (typeof navigator !== "undefined" && navigator.storage &&
            typeof navigator.storage.getDirectory === "function") {
            return await OpfsStore.create(url, blockLength);
        }
    } catch (e) {
        console.warn("OPFS not available, falling back to in-memory store", e);
    }
    return new NoStore();
}

export function readUint32(container: Uint8Array, offset: number) {
    return ((container[offset] & 0x000000FF) |
        ((container[offset + 1] << 8) & 0x0000FF00) |
        ((container[offset + 2] << 16) & 0x00FF0000) |
        ((container[offset + 3] << 24) & 0xFF000000)) >>> 0;
}

export function writeUint32(container: Uint8Array, value: number, offset: number) {
    container[offset] = value & 0xFF;
    container[offset + 1] = (value & 0x0000FF00) >> 8;
    container[offset + 2] = (value & 0x00FF0000) >> 16;
    container[offset + 3] = (value & 0xFF000000) >> 24;
    return offset + 4;
}
