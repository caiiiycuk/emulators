#include <emscripten.h>
#include <protocol.h>
#include <timer.h>

#include <cstdio>
#include <cstdlib>
#include <string>

NetworkType connectNetwork = NETWORK_NA;
std::string connectToAddress("");


int frameHeight = 0;
int frameWidth = 0;

// clang-format off
EM_JS(void, ws_init_runtime, (const char* sessionId), {
    var worker = typeof importScripts === "function";
    Module.messageSent = 0;
    Module.messageReceived = 0;
    Module.messageFrame = 0;
    Module.messageSound = 0;
    Module.sessionId = UTF8ToString(sessionId);
    Module.bundles = [];
    Module.files = {};
    Module.FS.ignorePermissions = true;

    function fsTree(root, parent) {
      for (const name of Object.keys(root)) {
        const fsNode = root[name];
        const folder = fsNode.isFolder;
        const node = {
          name,
        };

        if (folder) {
          node.nodes = [];
          node.size = null;
        } else {
          node.nodes = null;
          node.size = fsNode.usedBytes;
        }
        
        parent.nodes.push(node);

        if (folder) {
            fsTree(fsNode.contents, node);
        }
      }

      return parent;
    }

    function sendMessage(name, props, transfer) {
      ++Module.messageSent;

      props = props || {};
      props.sessionId = Module.sessionId;
      if (Module.postMessage) {
        Module.postMessage(name, props, transfer);
      } else if (worker) {
        postMessage({ name, props }, transfer);
      } else {
        window.postMessage({ name, props }, "*", transfer);
      }
    };
    Module.sendMessage = sendMessage;
    Module.ping = function(msg) {
    };
    Module.log = function(message) {
      sendMessage("ws-log", { tag: "worker", message });
    };
    Module.warn = function(message) {
      sendMessage("ws-warn", { tag: "worker", message });
    };
    Module.err = function(message) {
      sendMessage("ws-err", { tag: "panic", message });
    };
    Module.print = Module.log;
    Module.printErr = Module.err;
    Module.mallocString = function(value) {
      const valueLength = Module['lengthBytesUTF8'](value) + 1;
      const valueBuffer = Module['_malloc'](valueLength);
      Module.stringToUTF8(value, valueBuffer, valueLength);
      return valueBuffer;
    };
    Module.withString = function(value, callback) {
      const cValue = Module.mallocString(value);
      callback(cValue);
      Module['_free'](cValue);
    };
    Module.uncaught = function (error) {
      if (error.message !== "Program terminated with exit(0)" &&
          error !== "Program terminated with exit(0)") {
        Module.err("Backend crashed, cause: " + (error.message || error));
      }
      if (Module.cleanup) {
        Module.cleanup();
      }
      if (Module.exit) {
        Module.exit();
      } else {
        Module.sendMessage("ws-exit");
      }
    };

    function messageHandler(e) {
      var data = e.data;

      if (data.name === undefined || data.name.length < 3 ||
          data.name[0] !== "w" || data.name[1] !== "c" || data.name[2] !== "-") {
        return;
      }

      if (data.props.sessionId !== Module.sessionId) {
        return;
      }

      processMessage(data);
    }

    function processMessage(data) {
      ++Module.messageReceived;

      switch (data.name) {
        case "wc-run": {
          Module.token = data.props.token || "";
          Module._extractBundleToFs();
          Module._runRuntime();
          sendMessage("ws-server-ready");
        } break;
        case "wc-pause": {
          Module._requestPause();
        } break;
        case "wc-resume": {
          Module._requestResume();
        } break;
        case "wc-mute": {
          Module._requestMute();
        } break;
        case "wc-unmute": {
          Module._requestUnmute();
        } break;
        case "wc-exit": {
          Module._requestExit();
        } break;
        case "wc-pack-fs-to-bundle": {
          try {
            Module.persist = function(archive) {
              if (archive === null) {
                sendMessage("ws-persist", { bundle: null });
              } else {
                sendMessage("ws-persist", { bundle: archive }, [ archive.buffer ]);
              }
            };
            Module._packFsToBundle(data.props.onlyChanges);
            delete Module.persist;
          } catch (e) {
            Module.err(e.message);
          }
        } break;
        case "wc-add-key": {
          Module._addKey(data.props.key, data.props.pressed, data.props.timeMs);
        } break;
        case "wc-mouse-move": {
          Module._mouseMove(data.props.x, data.props.y, data.props.relative, data.props.timeMs);
        } break;
        case "wc-mouse-button": {
          Module._mouseButton(data.props.button, data.props.pressed, data.props.timeMs);
        } break;
        case "wc-mouse-sync": {
          Module._mouseSync(data.props.timeMs);
        } break;
        case "wc-sync-sleep": {
          // ignore
        } break;
        case "wc-backend-event": {
          if (Module.onBackendEvent) {
            Module.onBackendEvent(data.props.json);
          } else {
            Module.err("Backend does not support custom events");
          }
        } break;
        case "wc-connect": {
          const buffer = Module.mallocString(data.props.address);
          Module._networkConnect(data.props.networkType, buffer);
          Module._free(buffer);
        } break;
        case "wc-disconnect": {
          Module._networkDisconnect(data.props.networkType);
        } break;
        case "wc-asyncify-stats": {
          const stats = {
            messageSent: Module.messageSent,
            messageReceived: Module.messageReceived,
            messageFrame: Module.messageFrame,
            messageSound: Module.messageSound,
            nonSkippableSleepCount: Module.nonskippable_sleep_count,
            sleepCount: Module.sleep_count,
            sleepTime: Module.sleep_time,
            cycles: Module.cycles,
            netSent: Module.netSent || 0,
            netRecv: Module.netRecv || 0,
            driveSent: 0,
            driveRecv: 0,
            driveRecvTime: 0,
            driveCacheHit: 0,
            driveCacheMiss: 0,
            driveCacheUsed: 0,
          };

          if (Module.sockdrive && Module.sockdrive.stats) {
            stats.driveSent = Module.sockdrive.stats.write;
            stats.driveRecv = Module.sockdrive.stats.read;
            stats.driveRecvTime = Module.sockdrive.stats.readTotalTime;
            stats.driveCacheHit = Module.sockdrive.stats.cacheHit;
            stats.driveCacheMiss = Module.sockdrive.stats.cacheMiss;
            stats.driveCacheUsed = Module.sockdrive.stats.cacheUsed;
          }
          
          sendMessage("ws-asyncify-stats", stats);
        } break;
        case "wc-fs-tree": {
          sendMessage("ws-fs-tree", {
            fsTree: fsTree(Module.FS.root.contents.home.contents.web_user.contents, {
              name: ".",
              nodes: [],
              size: null,
            }),
          });
        } break;
        case "wc-send-data-chunk": {
          function mergeChunks(parts) {
              if (parts.length === 1) {
                  return parts[0];
              }

              let length = 0;
              for (const next of parts) {
                  length += next.byteLength;
              }
              const merged = new Uint8Array(length);
              length = 0;
              for (const next of parts) {
                  merged.set(next, length);
                  length += next.byteLength;
              }
              return merged;
          }

          function createPath(parts, begin, end) {
              let path = "/home/web_user";
              for (let i = begin; i < end; ++i) {
                  const part = parts[i].trim();
                  if (part.length === 0) {
                      continue;
                  }

                  Module.FS.createPath(path, part, true, true);
                  path = path + "/" + part;
              }

              return path;
          }
          
          const chunk = data.props.chunk;
          if (chunk.type === "bundle") {
            const index = Number.parseInt(chunk.name);
            if (Module.bundles[index] === undefined) {
              Module.bundles[index] = [];
            }

            if (chunk.data === null) {
              Module.bundles[index] = mergeChunks(Module.bundles[index]);
            } else {
              Module.bundles[index].push(new Uint8Array(chunk.data));
            }

            sendMessage("ws-send-data-chunk", {
              chunk: {
                type: "ok",
                name: chunk.name,
                data: null,
              },
            });
          } else if (chunk.type === "file") {
            const file = chunk.name;
            if (Module.files[file] === undefined) {
              Module.files[file] = [];
            }

            if (chunk.data === null) {
              const body = mergeChunks(Module.files[file]);
              delete Module.files[file];

              const parts = file.split("/");
              if (parts.length === 0) {
                Module.err("Can't create file '" + file + "', because it's not valid file path");
                return;
              }
              const filename = parts[parts.length - 1].trim();
              const path = createPath(parts, 0, parts.length - 1);
              Module.FS.writeFile(path + "/" + filename, body);
            } else {
              Module.files[file].push(new Uint8Array(chunk.data));
            }

            sendMessage("ws-send-data-chunk", {
              chunk: {
                type: "ok",
                name: chunk.name,
                data: null,
              },
            });
          } else {
            Module.err("Unknown chunk type: " + chunk.type);
          }
        } break;
        case "wc-fs-get-file": {
          const file = data.props.file;
          const contents = Module.FS.readFile("/home/web_user/" + file, { encoding: "binary" });
          sendMessage("ws-send-data-chunk", {
            chunk: {
              type: "file",
              name: file,
              data: contents.buffer,
            },
          });
          sendMessage("ws-send-data-chunk", {
            chunk: {
              type: "file",
              name: file,
              data: null,
            },
          });
        } break;
        default: {
          console.log("Unknown client message (wc): " + JSON.stringify(data));
        } break;
      }
    };

    if (Module.postMessage) {
      Module.messageHandler = messageHandler;
      Module.cleanup = function() { /**/ };
    } else if (worker) {
      onmessage = messageHandler;
      Module.cleanup = function() { /**/ };
    } else {
      window.addEventListener("message", messageHandler, { passive: true });
      Module.cleanup = function () {
        window.removeEventListener("message", messageHandler);
      }
    }

    sendMessage("ws-ready", {});
  });

EM_JS(void, emsc_ws_client_frame_set_size, (int width, int height), {
    Module.sendMessage("ws-frame-set-size", {width : width, height : height});
  });

EM_JS(bool, emsc_start_frame_update, (void* rgba), {
    Module.frame_update_lines = [];
    Module.frame_update_lines_transferable = [];
    return true;
  });

EM_JS(void, emsc_add_frame_line, (uint32_t start, char* ptr, uint32_t bpp4len), {
    var bpp3 = new Uint8Array(bpp4len / 4 * 3);
    var bpp4 = Module.HEAPU8;

    var bpp3Offset = 0;
    var bpp4Offset = ptr;
    while (bpp3Offset < bpp3.length) {
      bpp3[bpp3Offset++] = bpp4[bpp4Offset++];
      bpp3[bpp3Offset++] = bpp4[bpp4Offset++];
      bpp3[bpp3Offset++] = bpp4[bpp4Offset++];
      bpp4Offset++;
    }

    Module.frame_update_lines.push({start : start, heapu8 : bpp3});
    Module.frame_update_lines_transferable.push(bpp3.buffer);
  });

EM_JS(void, emsc_end_frame_update, (), {
    if (Module.frame_update_lines.length > 0) {
      ++Module.messageFrame;
      Module.sendMessage("ws-update-lines", 
        { lines: Module.frame_update_lines },
        Module.frame_update_lines_transferable);
    }
    delete Module.frame_update_lines;
    delete Module.frame_update_lines_transferable;
  });

EM_JS(void, emsc_ws_client_sound_init, (int freq), {
    Module.sendMessage("ws-sound-init", { freq : freq });
  });

EM_JS(void, emsc_ws_client_sound_push, (const float *samples, int num_samples), {
    if (num_samples <= 0) {
        return;
    }
  
    ++Module.messageSound;
    const heapf32 = Module.HEAPF32.slice(samples / 4, samples / 4 + num_samples);
      Module.sendMessage("ws-sound-push", 
        { samples: heapf32 },
        [ heapf32.buffer ]);
  });

EM_JS(void, emsc_ws_exit_runtime, (), {
    Module.exit = function () {
      Module.sendMessage("ws-exit");
    }
  });

EM_JS(void, ws_client_stdout, (const char* data, uint32_t amount), {
    Module.sendMessage("ws-stdout", { message: UTF8ToString(data, amount) });
  });

EM_JS(void, ws_client_log, (const char* tag, const char* message), {
    Module.sendMessage("ws-log", { tag: UTF8ToString(tag), message: UTF8ToString(message) });
  });

EM_JS(void, ws_client_warn, (const char* tag, const char* message), {
    Module.sendMessage("ws-warn", { tag: UTF8ToString(tag), message: UTF8ToString(message) });
  });

EM_JS(void, ws_client_error, (const char* tag, const char* message), {
    Module.sendMessage("ws-err", { tag: UTF8ToString(tag), message: UTF8ToString(message) });
  });

EM_JS(void, ws_client_network_connected, (NetworkType networkType, const char* address), {
    Module.sendMessage("ws-connected", { networkType, address: UTF8ToString(address) });
  });

EM_JS(void, ws_client_network_disconnected, (NetworkType networkType), {
    Module.sendMessage("ws-disconnected", { networkType });
  });

EM_JS(void, emsc_exit_runtime, (), {
    if (!Module.exit) {
      var message = "ERR! exitRuntime called without request" +
                    ", asyncify state: " + Asyncify.state;
      Module.err(message);
      return;
    }
    Module.exit();
    Module.cleanup();
  });

EM_JS(void, emsc_extract_bundle_to_fs, (), {
    Module.FS.chdir("/home/web_user");

    let index;
    Module.libzip_progress = function(file, extracted, count) {
        Module.sendMessage("ws-extract-progress", { index, file, extracted, count });
    };

    let dosboxConf = null;
    for (index = 0; index < Module.bundles.length; ++index) {
      const bytes = Module.bundles[index];
      const buffer = Module._malloc(bytes.length);
      Module.HEAPU8.set(bytes, buffer);
      const retcode = Module._zip_to_fs(buffer, bytes.length, 0);
      Module._free(buffer);

      if (retcode !== 0) {
        Module.err("Unable to extract bundle archive\n");
        return;
      }

        
      if (index === 0) {
        try {
            dosboxConf = Module.FS.readFile("/home/web_user/.jsdos/dosbox.conf");
        } catch (e) {
            Module.err("Broken bundle, .jsdos/dosbox.conf not found");
            return;
        }
        
        Module.fsCreatedAt = Module._get_changes_mtime_ms();
      }
    }

    const configContentPtr = Module._getConfigContent();
    const configContent = Module.UTF8ToString(configContentPtr);
    Module._free(configContentPtr);
    Module.sendMessage("ws-config", {
      dosboxConf,
      jsdosConf: configContent,
    });

    delete Module.libzip_progress;
    delete Module.bundles;
  });

EM_JS(void, emsc_pack_fs_to_bundle, (bool onlyChanges), {
    Module.FS.chdir("/home/web_user");

    const ptr = Module._zip_from_fs(onlyChanges ? Module.fsCreatedAt : 0);
    if (ptr === 0) {
      if (onlyChanges) {
        Module.persist(null);
      } else {
        Module.err("Can't create zip, see more info in logs");
        Module._abort();
      }
      return;
    }

    const length = Module.HEAPU32[ptr / 4];
    const memory = Module.HEAPU8;
    const archive = memory.slice(ptr + 4, ptr + 4 + length);
    Module._free(ptr);

    Module.persist(archive);
  });
// clang-format on

void client_frame_set_size(int width, int height) {
  frameHeight = height;
  frameWidth = width;
  emsc_ws_client_frame_set_size(width, height);
}

void client_frame_update_lines(uint32_t *lines, uint32_t batchCount, void *rgba) {
  if (!emsc_start_frame_update(rgba)) {
      return;
  }

  for (uint32_t i = 0; i < batchCount; ++i) {
    uint32_t base = i * 3;
    uint32_t start = lines[base];
    uint32_t count = lines[base + 1];
    uint32_t offset = lines[base + 2];
    emsc_add_frame_line(start, (char *)rgba + offset,
                        sizeof(uint32_t) * count * frameWidth);
  }
  emsc_end_frame_update();
}

void client_stdout(const char* data, uint32_t amount) {
  ws_client_stdout(data, amount);
}

void client_log(const char* tag, const char* message) {
  ws_client_log(tag, message);
}

void client_warn(const char* tag, const char* message) {
  ws_client_warn(tag, message);
}

void client_error(const char* tag, const char* message) {
  ws_client_error(tag, message);
}

void client_sound_init(int freq) {
  emsc_ws_client_sound_init(freq);
}

void client_sound_push(const float *samples, int num_samples) {
  emsc_ws_client_sound_push(samples, num_samples);
}

void client_network_connected(NetworkType networkType, const char* address) {
  ws_client_network_connected(networkType, address);
}

void client_network_disconnected(NetworkType networkType) {
  ws_client_network_disconnected(networkType);
}

extern "C" void EMSCRIPTEN_KEEPALIVE networkConnect(NetworkType networkType, const char* address) {
  connectNetwork = networkType;
  connectToAddress = address;
}

extern "C" void EMSCRIPTEN_KEEPALIVE networkDisconnect(NetworkType networkType) {
  connectNetwork = NETWORK_NA;
  connectToAddress = "";
  server_network_disconnect(networkType);
}

extern "C" void EMSCRIPTEN_KEEPALIVE extractBundleToFs() {
  emsc_extract_bundle_to_fs();
}

extern "C" void EMSCRIPTEN_KEEPALIVE packFsToBundle(bool onlyChanges) {
  emsc_pack_fs_to_bundle(onlyChanges);
}

extern "C" void EMSCRIPTEN_KEEPALIVE addKey(KBD_KEYS key, bool pressed, uint64_t timeMs) {
  server_add_key(key, pressed, timeMs);
}

extern "C" void EMSCRIPTEN_KEEPALIVE mouseMove(float x, float y, bool relative, uint64_t movedMs) {
  server_mouse_moved(x, y, relative, movedMs);
}

extern "C" void EMSCRIPTEN_KEEPALIVE mouseButton(int button, bool pressed, uint64_t pressedMs) {
  server_mouse_button(button, pressed, pressedMs);
}

extern "C" void EMSCRIPTEN_KEEPALIVE mouseSync(uint64_t syncMs) {
  server_mouse_sync(syncMs);
}

extern "C" void EMSCRIPTEN_KEEPALIVE exitRuntime() {
  emsc_exit_runtime();
}

void client_tick() {
  static bool reentranceLock = false;
  if (reentranceLock) {
    return;
  }

  reentranceLock = true;
  if (connectNetwork != NETWORK_NA) {
    server_network_connect(connectNetwork, connectToAddress.c_str());
    connectNetwork = NETWORK_NA;
    connectToAddress = "";
  }
  reentranceLock = false;
}

extern "C" void EMSCRIPTEN_KEEPALIVE runRuntime() {
  server_run();
  emsc_ws_exit_runtime();
  exitRuntime();
  emscripten_force_exit(0);
}

extern "C" void EMSCRIPTEN_KEEPALIVE requestPause() { 
  server_pause(); 
}

extern "C" void EMSCRIPTEN_KEEPALIVE requestResume() { 
  server_resume(); 
}

extern "C" void EMSCRIPTEN_KEEPALIVE requestMute() { 
  server_mute(); 
}

extern "C" void EMSCRIPTEN_KEEPALIVE requestUnmute() { 
  server_unmute(); 
}

extern "C" void EMSCRIPTEN_KEEPALIVE requestExit() { 
  server_exit(); 
}

extern "C" char* EMSCRIPTEN_KEEPALIVE getConfigContent() {
  FILE *f = fopen(".jsdos/jsdos.json", "rb");
  if (!f) {
    char *content = (char *) malloc(3);
    content[0] = '{';
    content[1] = '}';
    content[2] = 0;
    return content;
  }

  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *content = (char *) malloc(fsize + 1);
  fread(content, 1, fsize, f);
  fclose(f);

  content[fsize] = 0;
  return content;
}

int main(int argc, char **argv) {
  ws_init_runtime(argc > 1 ? argv[1] : "id-null");
  emscripten_exit_with_live_runtime();
  return 0;
}
