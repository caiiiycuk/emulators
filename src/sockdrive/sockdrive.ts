import { getStore, readUint32 } from "./sockdrive-store";
import { compress, compressBound, uncompress } from "../protocol/mini-lz4";

const BATCH_SIZE = 4;
const context: any = typeof window !== "undefined" ? window : typeof self !== "undefined" ? self : global;

interface DriveInfo {
    ahead_read: number;
    range_count: number;
    dropped_ranges: number[];
    preload_ranges: number[] | "_";
    small_ranges: number[];
    cylinders: number;
    heads: number;
    sectors: number;
    sector_size: number;
    size: number;
    name: string;
    url: string;

    preloadSizeInBytes: number;
    sizeInBytes: number;
    readInBytes: number;
    writeInBytes: number;
}

export interface Drive {
    info: DriveInfo;
    range(sector: number): number;
    readRangeAsync(range: number): void;
    ready(): void;
    write(sector: number, buffer: Uint8Array): void;
    persist(): Promise<Uint8Array | null>;
}

export async function sockdrive(
    url: string,
    persistedSectors: Uint8Array | null,
    preloadMode: "all" | "default" | "none",
    _onNewRange: (range: number, buffer: Uint8Array) => void,
): Promise<Drive> {
    const response = await fetch(url + "/sockdrive.metaj");
    const info = await response.json() as DriveInfo;
    const store = await getStore(url, info.ahead_read);
    try {
        info.preload_ranges = await (await fetch(url + "/preload_ranges.metaj")).json();
    } catch (e) {
        // ignore
    }
    info.url = url;
    info.readInBytes = 0;
    info.writeInBytes = 0;
    const loadedRanges: Set<number> = new Set();

    if (info.small_ranges === undefined) {
        info.small_ranges = [];
    }

    let storedSectors = new Map<number, Map<number, Uint8Array>>();
    if (persistedSectors) {
        info.writeInBytes = persistedSectors.length;
        storedSectors = deserializeSectors(persistedSectors);
    }

    const storeKeys = new Set<number>();
    for (const key of await store.keys()) {
        storeKeys.add(key);
    }

    if (info.small_ranges.find((range) => !storeKeys.has(range)) !== undefined) {
        const preload = new Uint8Array(await (await fetch(url + "/preload.raw")).arrayBuffer());

        for (let i = 0; i < info.small_ranges.length; i++) {
            const range = info.small_ranges[i];
            storeKeys.add(range);
            store.put(range, preload.slice(i * info.ahead_read, (i + 1) * info.ahead_read)).catch(console.error);
        };
    }

    const loaded = new Set<number>();
    const droppedRanges: number[] = info.dropped_ranges;
    const emptyRanges: Set<number> = new Set();
    info.dropped_ranges = [];
    for (const next of droppedRanges) {
        emptyRanges.add(next);
        if (!storedSectors.has(next)) {
            loaded.add(next);
            info.dropped_ranges.push(next);
        }
    }

    const loadQueue: number[] = [];
    const preloaded = new Set<number>();
    if (preloadMode === "none") {
        console.warn("Preload mode is disabled");
    } else if (info.preload_ranges !== "_" && preloadMode !== "all") {
        for (const next of info.preload_ranges) {
            if (!loaded.has(next)) {
                loadQueue.push(next);
                preloaded.add(next);
            }
        }
    } else {
        for (let i = 0; i < info.range_count; i++) {
            if (!loaded.has(i)) {
                loadQueue.push(i);
            }
        }
    }

    // validate queue
    {
        const invalidRanges = [];
        for (const range of loadQueue) {
            if (range < 0 || range >= info.range_count) {
                invalidRanges.push(range);
            }
        }
        if (invalidRanges.length > 0) {
            console.error("sockdrive-error: invalid ranges", invalidRanges);
            for (const range of invalidRanges) {
                loadQueue.splice(loadQueue.indexOf(range), 1);
            }
        }
    }

    loadQueue.reverse();


    let rangesToLoad = loadQueue.length;
    for (let i = 0; i < info.range_count; i++) {
        if (!loaded.has(i) && !preloaded.has(i)) {
            rangesToLoad++;
        }
    }
    info.preloadSizeInBytes = loadQueue.length * info.ahead_read;
    info.sizeInBytes = rangesToLoad * info.ahead_read;

    function range(sector: number) {
        return Math.floor(sector * info.sector_size / info.ahead_read);
    };

    function mergeSectorsData(range: number, data: Uint8Array): Uint8Array {
        if (storedSectors.has(range)) {
            for (const [offset, sectorData] of storedSectors.get(range)!.entries()) {
                data.set(sectorData, offset * info.sector_size);
            }
        }

        return data;
    }

    function onNewRange(range: number, buffer: Uint8Array) {
        _onNewRange(range, mergeSectorsData(range, buffer));
    }

    async function loadRange(range: number) {
        try {
            if (emptyRanges.has(range)) {
                onNewRange(range, new Uint8Array(info.ahead_read));
                return;
            }

            if (storeKeys.has(range)) {
                let buffer = store.getSync(range);
                if (buffer === null) {
                    buffer = await store.getAsync(range);
                }
                if (buffer) {
                    onNewRange(range, buffer);
                    return;
                }
            }

            let retries = 3;
            while (retries > 0) {
                let response;
                try {
                    response = await fetch(url + "/" + range + ".raw");
                } catch (e) {
                    // ignore
                }

                if (!response || !response.ok) {
                    retries--;
                    if (retries > 0) {
                        console.warn("Can't read range " + range + ", network response code is " +
                            response?.status + ", retrying...");
                        continue;
                    } else {
                        throw new Error("Can't read range " + range + ", network response code is " +
                            response?.status);
                    }
                }

                const buffer = new Uint8Array(await response.arrayBuffer());
                store.put(range, buffer).catch(console.error);
                onNewRange(range, buffer);
                break;
            }
        } catch (e) {
            console.error("Can't read range", range, e);
        } finally {
            info.readInBytes += info.ahead_read;
        }
    }

    async function loadFromQueue() {
        const bach: Promise<void>[] = [];
        while (loadQueue.length > 0 && bach.length < BATCH_SIZE) {
            const range = loadQueue.pop()!;
            if (!loaded.has(range)) {
                loaded.add(range);
                bach.push(loadRange(range));
            }
        }
        await Promise.all(bach);

        if (loadQueue.length > 0) {
            loadFromQueue().catch(console.error);
        }
    };

    function serializeSectors(sectors: Map<number, Map<number, Uint8Array>>): Uint8Array {
        const sectorsData = new Map<number, Uint8Array>();

        for (const [range, rangeSectors] of sectors.entries()) {
            rangeSectors.forEach((data, sector) => {
                sectorsData.set(sector + range * info.ahead_read / info.sector_size, data);
            });
        }

        const chunksSize = info.sector_size + 4;
        const boundSize = compressBound(chunksSize);

        const uncompressedChunk = new Uint8Array(chunksSize);
        const u32uncompressedChunk = new Uint32Array(uncompressedChunk.buffer);

        const compressedChunk = new Uint8Array(boundSize);
        const chunks: Uint8Array[] = [];

        let total = 0;
        sectorsData.forEach((data, sector) => {
            u32uncompressedChunk[0] = sector;
            uncompressedChunk.set(data, 4);
            const compressedSize = compress(uncompressedChunk, compressedChunk, 0, compressedChunk.length);
            if (compressedSize <= 0 || compressedSize >= uncompressedChunk.length) {
                chunks.push(uncompressedChunk.slice(0));
                total += chunksSize;
            } else {
                chunks.push(compressedChunk.slice(0, compressedSize));
                total += compressedSize;
            }
        });

        const payload = new Uint8Array(total + sectorsData.size * 4 + 4);
        payload[0] = sectorsData.size;
        payload[1] = (sectorsData.size & 0x0000ff00) >> 8;
        payload[2] = (sectorsData.size & 0x00ff0000) >> 16;
        payload[3] = (sectorsData.size & 0xff000000) >> 24;


        let offset = 4;
        for (const chunk of chunks) {
            payload[offset] = chunk.length;
            payload[offset + 1] = (chunk.length & 0x0000ff00) >> 8;
            payload[offset + 2] = (chunk.length & 0x00ff0000) >> 16;
            payload[offset + 3] = (chunk.length & 0xff000000) >> 24;
            offset += 4;
            payload.set(chunk, offset);
            offset += chunk.length;
        }

        return payload;
    }

    function deserializeSectors(data: Uint8Array): Map<number, Map<number, Uint8Array>> {
        const sectors = new Map<number, Map<number, Uint8Array>>();
        const count = (data[0] & 0x000000ff) | ((data[1] << 8) & 0x0000ff00) |
            ((data[2] << 16) & 0x00ff0000) | ((data[3] << 24) & 0xff000000);
        const chunkSize = info.sector_size + 4;
        const uncompressedChunk = new Uint8Array(chunkSize);
        const u32uncompressedChunk = new Uint32Array(uncompressedChunk.buffer);

        let offset = 4;
        for (let i = 0; i < count; i++) {
            const compressedSize = (data[offset] & 0x000000ff) | ((data[offset + 1] << 8) & 0x0000ff00) |
                ((data[offset + 2] << 16) & 0x00ff0000) | ((data[offset + 3] << 24) & 0xff000000);
            offset += 4;
            const compressedChunk = data.slice(offset, offset + compressedSize);
            offset += compressedSize;


            let sector;
            let sectorData;
            if (compressedSize === chunkSize) {
                sector = new Uint32Array(compressedChunk.buffer)[0];
                sectorData = compressedChunk.slice(4);
            } else {
                const uncompressedSize = uncompress(compressedChunk, uncompressedChunk, 0, 0);
                if (uncompressedSize !== chunkSize) {
                    console.error("Can't uncompress sectors data, size mismatch",
                        uncompressedSize, "!==", chunkSize, "chunk", i, "offset", offset);
                    return new Map();
                }
                sector = u32uncompressedChunk[0];
                sectorData = uncompressedChunk.slice(4);
            }


            const rangeOfSector = range(sector);

            if (!sectors.has(rangeOfSector)) {
                sectors.set(rangeOfSector, new Map());
            }

            sectors.get(rangeOfSector)!.set(
                sector - (rangeOfSector * info.ahead_read) / info.sector_size,
                sectorData,
            );
        }

        return sectors;
    }

    context.loadedRanges = () => {
        console.log("Loaded ranges (" + loadedRanges.size + "):",
            JSON.stringify(Array.from(loadedRanges).sort((a, b) => a - b)));
    };

    context.brotliRanges = () => {
        console.log("Loaded ranges (" + loadedRanges.size + "):",
            "echo '" +
            JSON.stringify(Array.from(loadedRanges).sort((a, b) => a - b)) +
            "' | brotli > /tmp/preload_ranges.metaj");
    };

    context.verifySectors = () => {
        if (storedSectors.size === 0) {
            return;
        }

        const serialized = serializeSectors(storedSectors);
        // Create a blob URL for downloading the serialized data
        const blob = new Blob([serialized], { type: "application/octet-stream" });
        const url = URL.createObjectURL(blob);
        console.log("Download serialized sectors:", url);
        const deserialized = deserializeSectors(serialized);
        console.log("Comparing sectors and deserialized:");

        // Compare sizes
        console.log("Original sectors size:", storedSectors.size);
        console.log("Deserialized sectors size:", deserialized.size);

        // Compare each range and sector
        storedSectors.forEach((rangeMap, rangeKey) => {
            const deserializedRange = deserialized.get(rangeKey);
            if (!deserializedRange) {
                console.error(`Range ${rangeKey} missing in deserialized data`);
                return;
            }

            rangeMap.forEach((sectorData, sectorKey) => {
                const deserializedSector = deserializedRange.get(sectorKey);
                if (!deserializedSector) {
                    console.error(`Sector ${sectorKey} missing in range ${rangeKey}`);
                    return;
                }

                // Compare sector data
                const match = sectorData.length === deserializedSector.length &&
                    sectorData.every((val, i) => val === deserializedSector[i]);

                if (!match) {
                    console.error(`Data mismatch in range ${rangeKey}, sector ${sectorKey}`);
                    console.log("Original:", sectorData);
                    console.log("Deserialized:", deserializedSector);
                }
            });
        });
    };

    return {
        info,
        range,
        readRangeAsync: async (range: number) => {
            loadedRanges.add(range);
            if (!loaded.has(range)) {
                loaded.add(range);
                loadRange(range);
            }
        },
        ready: () => {
            loadFromQueue().catch(console.error);
        },
        write: (sector: number, buffer: Uint8Array) => {
            const rangeOfSector = range(sector);
            if (!storedSectors.has(rangeOfSector)) {
                storedSectors.set(rangeOfSector, new Map());
            }
            storedSectors.get(rangeOfSector)!.set(
                sector - (rangeOfSector * info.ahead_read) / info.sector_size,
                buffer);
        },
        persist: async () => {
            const serialized = serializeSectors(storedSectors);
            if (serialized.byteLength > 4) {
                return serialized;
            }
            return null;
        },
    };
}

export function traverseSockdriveChanges(encoded: Uint8Array,
                                         callback: (url: string, changes: Uint8Array) => void) {
    const decoder = new TextDecoder();
    let offset = 0;
    while (offset < encoded.length) {
        const urlLength = readUint32(encoded, offset);
        offset += 4;

        if (urlLength > 4096) {
            return false;
        }

        const url = decoder.decode(encoded.slice(offset, offset + urlLength));

        if (!(url.startsWith("http://") || url.startsWith("https://"))) {
            return false;
        }

        offset += urlLength;

        const persistLength = readUint32(encoded, offset);
        offset += 4;

        const changes = encoded.slice(offset, offset + persistLength);
        offset += persistLength;
        callback(url, changes);
    }

    return true;
}

context.sockdrive = sockdrive;
context.traverseSockdriveChanges = traverseSockdriveChanges;
