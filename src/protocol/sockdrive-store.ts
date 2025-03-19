export const RAW_STORE = "raw";
export const WRITE_STORE = "write";

export interface Store {
    put: (key: number, data: Uint8Array, store: string) => Promise<void>;
    get: (key: number, store: string) => Promise<Uint8Array | null>;
    keys: (store: string) => Promise<number[]>;
    each: (key: number[], store: string, callback: (key: number, data: Uint8Array) => void) => Promise<void>;
    close: () => void;
}

export class NoStore implements Store {
    public owner = "";

    public close() {
    }

    public put(key: number, data: Uint8Array, store: string): Promise<void> {
        return Promise.resolve();
    }

    public get(range: number, store: string): Promise<Uint8Array | null> {
        return Promise.resolve(null);
    }

    public keys(store: string): Promise<number[]> {
        return Promise.resolve([]);
    }

    public each(keys: number[], store: string, callback: (key: number, data: Uint8Array) => void) {
        return Promise.resolve();
    }
}

class DbStore implements Store {
    private indexedDB: IDBFactory;
    private db: IDBDatabase | null = null;

    constructor(
        url: string,
        onready: (cache: Store) => void,
        onerror: (msg: string) => void) {
        this.indexedDB = (typeof window === "undefined" ? undefined : window.indexedDB ||
            (window as any).mozIndexedDB ||
            (window as any).webkitIndexedDB || (window as any).msIndexedDB) as any;

        if (!this.indexedDB) {
            onerror("IndexedDB is not supported on this host");
            return;
        }

        try {
            const openRequest = this.indexedDB.open("sockdrive (" + url + ")", 1);
            openRequest.onerror = () => {
                onerror("Can't open cache database: " + openRequest.error?.message);
            };
            openRequest.onsuccess = () => {
                this.db = openRequest.result;
                onready(this);
            };
            openRequest.onupgradeneeded = () => {
                try {
                    this.db = openRequest.result;
                    this.db.onerror = () => {
                        onerror("Can't upgrade cache database");
                    };

                    this.db.createObjectStore(RAW_STORE)
                        .createIndex("range", "", { multiEntry: false });
                    this.db.createObjectStore(WRITE_STORE)
                        .createIndex("sector", "", { multiEntry: false });
                } catch (e) {
                    onerror("Can't upgrade cache database");
                }
            };
        } catch (e: any) {
            onerror("Can't open cache database: " + e.message);
        }
    }

    public close() {
        if (this.db !== null) {
            this.db.close();
            this.db = null;
        }
    }

    public put(key: number, data: Uint8Array, store: string): Promise<void> {
        return new Promise<void>((resolve) => {
            const transaction = this.db!.transaction(store, "readwrite");
            const request = transaction.objectStore(store).put(new Blob([data.buffer]), key);
            request.onerror = (e) => {
                console.error(e);
                resolve();
            };
            request.onsuccess = () => {
                resolve();
            };
        });
    }

    public get(key: number, store: string): Promise<Uint8Array | null> {
        return new Promise<Uint8Array | null>((resolve) => {
            const transaction = this.db!.transaction(store, "readonly");
            const request = transaction.objectStore(store).get(key) as IDBRequest<ArrayBuffer | Blob>;
            request.onerror = (e) => {
                console.error(e);
                resolve(null);
            };
            request.onsuccess = () => {
                if (request.result) {
                    (request.result as Blob).arrayBuffer().then((buffer) => {
                        resolve(new Uint8Array(buffer));
                    }).catch((e) => {
                        console.error(e);
                        resolve(null);
                    });
                } else {
                    resolve(null);
                }
            };
        });
    }

    public keys(store: string): Promise<number[]> {
        return new Promise<number[]>((resolve) => {
            if (this.db === null) {
                resolve([]);
                return;
            }

            const transaction = this.db.transaction(store, "readonly");
            const request = transaction.objectStore(store).getAllKeys();
            request.onerror = (e) => {
                console.error(e);
                resolve([]);
            };
            request.onsuccess = (event) => {
                if (request.result) {
                    resolve(request.result as number[]);
                } else {
                    resolve([]);
                }
            };
        });
    }

    public each(keys: number[], storeName: string, callback: (key: number, data: Uint8Array) => void) {
        return new Promise<void>((resolve) => {
            if (this.db === null) {
                resolve();
                return;
            }

            const transaction = this.db.transaction(storeName, "readonly");
            const store = transaction.objectStore(storeName);

            const readOne = async (key: number) => {
                return new Promise<Uint8Array>((resolve, reject) => {
                    const request = store.get(key);
                    request.onerror = (e) => {
                        reject(e);
                    };
                    request.onsuccess = (event) => {
                        (request.result as Blob).arrayBuffer()
                            .then((buffer) => {
                                resolve(new Uint8Array(buffer));
                            }).catch(reject);
                    };
                });
            };

            (async () => {
                for (const key of keys) {
                    const data = await readOne(key);
                    callback(key, data);
                }
                resolve();
            })().catch((e) => {
                console.error(e);
                resolve();
            });
        });
    }
}

export function getStore(owner: string): Promise<Store> {
    return new Promise((resolve) => {
        new DbStore(owner, resolve, (msg: string) => {
            console.error("Can't open IndexedDB cache", msg);
            resolve(new NoStore());
        });
    });
}
