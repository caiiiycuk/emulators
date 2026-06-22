#include <emscripten.h>
#include <protocol.h>
#include <timer.h>

#include <cstdio>
#include <cstdlib>
#include <string>

#include <sockdrive.h>
#include <filesystem>

NetworkType connectNetwork = NETWORK_NA;
std::string connectToAddress("");


int frameHeight = 0;
int frameWidth = 0;
uint8_t *frameRgb = nullptr;

// clang-format off
#include <filesystem>
EM_JS(void, ws_init_runtime, (const char* sessionId), {
    var worker = typeof importScripts === "function";
    Module.sockdrives = {};
    Module.sockdriveChanges = {};
    Module.worker = worker;
    Module.messageSent = 0;
    Module.messageReceived = 0;
    Module.messageFrame = 0;
    Module.messageSound = 0;
    Module.sessionId = UTF8ToString(sessionId);
    Module.bundles = [];
    Module.files = {};
    Module.FS.ignorePermissions = true;
    Module.driveIo = {};
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
    Module.onBackendEvent = function (json) {
        console.log("backend event", json);
        const message = JSON.parse(json);
        switch (message.type) {
            case "wc-trigger-event": {
              if (Module._TriggerEventByName) {
                // defined with MAPPER_AddHandler
                Module.withString(message.event, function(name) {
                    Module._TriggerEventByName(name);
                });
              } else {
                debugger;
                Module.err("Backend does not support custom events");
              }
            } break;
            default:
                Module.err("Unknown event: " + json);
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
      if (data.name !== "wc-sync-sleep") {
        ++Module.messageReceived;
      }

      switch (data.name) {
        case "wc-run": {
          if (data.props.myPeerId) {
            Module._setMyPeerId(data.props.myPeerId);
          }
          Module.token = data.props.token || "";
          Module.sockdrivePreload = data.props.sockdrivePreload || "default";
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
          Module.onBackendEvent(data.props.json);
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
          const driveIo = [];
          for (const drive of Object.values(Module.sockdrives)) {
              driveIo.push({
                  url: drive.info.url,
                  preload: drive.info.preloadSizeInBytes,
                  total: drive.info.sizeInBytes,
                  read: drive.info.readInBytes,
                  write: drive.info.writeInBytes,
              });
          }
          const stats = {
            glfx: !!Module.glfx,
            offscreenCanvas: !!Module.canvas,
            messageSent: Module.messageSent,
            messageReceived: Module.messageReceived,
            messageFrame: Module.messageFrame,
            messageSound: Module.messageSound,
            nonSkippableSleepCount: Module.nonskippable_sleep_count,
            sleepCount: Module.sleep_count,
            sleepTime: Module.sleep_time,
            cpuMetrics: Module.UTF8ToString(Module._getCPUMetrics()),
            netSent: Module.netSent || 0,
            netRecv: Module.netRecv || 0,
            driveIo,
          };

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
        case "wc-fs-delete-file": {
          Module.withString(data.props.file, (cstr) => {
            const deleted = Module._fsDeleteFile(cstr) === 1;
            sendMessage("ws-fs-delete-file", { deleted });
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
        case "wc-net-connected": {
          if (Module.wsNetConnectResolve) {
            Module.wsNetConnectResolve(data.props.networkId);
          } else {
            console.error("wc-net-connected recived but no awaiting promises, networkId:", data.props.networkId);
          }
        } break;
        case "wc-net-received": {
          const buffer = new Uint8Array(data.props.data);
          const ptr = Module._malloc(buffer.length);
          Module.HEAPU8.set(buffer, ptr);
          Module._ws_client_net_recv(data.props.peerId, ptr, buffer.length);
        } break;
        case "wc-unload": {
          if (Module.wsUnloadResolve) {
            Module.wsUnloadResolve();
          } else {
            console.error("wc-unload recived but no awaiting promises");
          }
        } break;
        case "wc-persist-sockdrives": {
          if (Object.keys(Module.sockdrives).length === 0) {
            sendMessage("ws-persist-sockdrives", { drives: null });
            return;
          }

          (async () => {
            const drives = [];
            // eslint-disable-next-line no-unused-vars
            for (const [_, drive] of Object.entries(Module.sockdrives)) {
                const persist = await drive.persist();
                if (persist !== null) {
                    drives.push({
                        url: drive.info.url,
                        persist,
                    });
                }
            }
            sendMessage("ws-persist-sockdrives", { drives });
          })().catch((e) => Module.err("Can't persist sockdrives: " + e.message));
        } break;
        case "wc-get-running-program": {
          const ptr = Module["_emGetRunningProgram"]();
          sendMessage("ws-get-running-program", { program: ptr !== 0 ? UTF8ToString(ptr) : "" });
        } break;
        default: {
          console.log("Unknown client message (wc): " + JSON.stringify(data));
        } break;
      }
    };

    Module.onSockdriveNewRange = (handle, range, buffer) => {
          const ptr = Module["_malloc"](buffer.length);
          Module.HEAPU8.set(buffer, ptr);
          Module["_em_client_sockdrive_new_range"](handle, range, ptr);
          Module["_free"](ptr);
    };

    Module.onSockdriveOpened = (handle, size, heads, cylinders, sectors, sectorSize, aheadRange, emptyRangesCount, emptyRanges) => {
      Module.sockdriveSectorSize = sectorSize;
      const ptr = Module["_malloc"](emptyRangesCount * 4);
      for (let i = 0; i < emptyRangesCount; ++i) {
        const value = emptyRanges[i];
        const offset = ptr + i * 4;
        Module.HEAPU8[offset] = value & 0xFF;
        Module.HEAPU8[offset + 1] = (value & 0x0000FF00) >> 8;
        Module.HEAPU8[offset + 2] = (value & 0x00FF0000) >> 16;
        Module.HEAPU8[offset + 3] = (value & 0xFF000000) >> 24;
      }
      Module["_em_client_sockdrive_opened"](handle, size, heads, cylinders, sectors, sectorSize, aheadRange, emptyRangesCount, ptr);
      Module["_free"](ptr);
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

    if (Module.canvas) {
      try {
        (function () {
          if (!Module.canvas.style) {
            Module.canvas.style = {};
          }
          if (worker) {
            self.screen = {
              width: 320,
              height: 200,
            };
            self.document = {
              querySelector: function() {
                return null;
              },
            };
          }

          const gl = Module.canvas.getContext("webgl", {
            alpha: false,
            antialias: false,
            depth: false,
            stencil: false,
            preserveDrawingBuffer: false,
            desynchronized: false,
            powerPreference: "high-performance",
          });
          if (!gl) {
            throw new Error("Unable to get WebGL context");
          }

          Module.preinitializedWebGLContext = gl;
          Module.gl = gl;
          Module.glfx = false;

          const vsSource = `
            attribute vec4 aVertexPosition;
            attribute vec2 aTextureCoord;

            varying highp vec2 vTextureCoord;

            void main(void) {
              gl_Position = aVertexPosition;
              vTextureCoord = aTextureCoord;
            }
          `;

          const fsSource = `
            varying highp vec2 vTextureCoord;
            uniform sampler2D uSampler;


            void main(void) {
              highp vec4 color = texture2D(uSampler, vTextureCoord);
              gl_FragColor = vec4(color.r, color.g, color.b, 1.0);
            }
          `;

          function loadShader(gl, shaderType, source) {
              const shader = gl.createShader(shaderType);
              gl.shaderSource(shader, source);
              gl.compileShader(shader);
              if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
                  const info = gl.getShaderInfoLog(shader);
                  gl.deleteShader(shader);
                  throw new Error("An error occurred compiling the shaders: " + info);
              }

              return shader;
          }

          function initShaderProgram(gl, vsSource, fsSource) {
              const vertexShader = loadShader(gl, gl.VERTEX_SHADER, vsSource);
              const fragmentShader = loadShader(gl, gl.FRAGMENT_SHADER, fsSource);

              const shaderProgram = gl.createProgram();
              gl.attachShader(shaderProgram, vertexShader);
              gl.attachShader(shaderProgram, fragmentShader);
              gl.linkProgram(shaderProgram);

              if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
                  throw new Error("Unable to initialize the shader program: " + gl.getProgramInfoLog(shaderProgram));
              }

              return shaderProgram;
          }

          const quadProgram = initShaderProgram(gl, vsSource, fsSource);
          const vertexPosition = gl.getAttribLocation(quadProgram, "aVertexPosition");
          const textureCoord = gl.getAttribLocation(quadProgram, "aTextureCoord");
          const uSampler = gl.getUniformLocation(quadProgram, "uSampler");

          const positionBuffer = gl.createBuffer();
          gl.bindBuffer(gl.ARRAY_BUFFER, positionBuffer);
          const positions = [
              -1.0, -1.0, 0.0,
              1.0, -1.0, 0.0,
              1.0, 1.0, 0.0,
              -1.0, -1.0, 0.0,
              1.0, 1.0, 0.0,
              -1.0, 1.0, 0.0,
          ];
          gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(positions), gl.STATIC_DRAW);

          const textureCoordBuffer = gl.createBuffer();
          gl.bindBuffer(gl.ARRAY_BUFFER, textureCoordBuffer);
          const textureCoordinates = [
              0.0, 1.0,
              1.0, 1.0,
              1.0, 0.0,
              0.0, 1.0,
              1.0, 0.0,
              0.0, 0.0,
          ];
          gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(textureCoordinates),
              gl.STATIC_DRAW);

          const textureCoordinatesFlippedBuffer = gl.createBuffer();
          const textureCoordinatesFlipped = [
              0.0, 0.0,
              1.0, 0.0,
              1.0, 1.0,
              0.0, 0.0,
              1.0, 1.0,
              0.0, 1.0,
          ];
          gl.bindBuffer(gl.ARRAY_BUFFER, textureCoordinatesFlippedBuffer);
          gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(textureCoordinatesFlipped), gl.STATIC_DRAW);

          const screenTexture = gl.createTexture();
          gl.bindTexture(gl.TEXTURE_2D, screenTexture);
          gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
          gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
          gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
          gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);

          const pixel = new Uint8Array([0, 0, 0]);
          gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGB,
              1, 1, 0, gl.RGB, gl.UNSIGNED_BYTE,
              pixel);

          function withQuadProgram(fn, flipped) {
            let binded = false;
            function bindQuadProgram() {
                if (binded) {
                  return;
                }

                gl.bindBuffer(gl.ARRAY_BUFFER, positionBuffer);
                gl.vertexAttribPointer(vertexPosition, 3, gl.FLOAT, false, 0, 0);
                gl.enableVertexAttribArray(vertexPosition);
                gl.bindBuffer(gl.ARRAY_BUFFER, flipped ? textureCoordinatesFlippedBuffer : textureCoordBuffer);
                gl.vertexAttribPointer(textureCoord, 2, gl.FLOAT, false, 0, 0);
                gl.enableVertexAttribArray(textureCoord);
                gl.useProgram(quadProgram);
                gl.activeTexture(gl.TEXTURE0);
                gl.uniform1i(uSampler, 0);
                binded = true;
            }

            if (Module.glfx) {
              const prevProgram = gl.getParameter(gl.CURRENT_PROGRAM);
              const prevActiveTexture = gl.getParameter(gl.ACTIVE_TEXTURE);
              const prevTexture = gl.getParameter(gl.TEXTURE_BINDING_2D);
              const prevArrayBuffer = gl.getParameter(gl.ARRAY_BUFFER_BINDING);
              const prevVertexAttribPosition = gl.getVertexAttrib(vertexPosition, gl.VERTEX_ATTRIB_ARRAY_ENABLED);
              const prevVertexAttribTextureCoord = gl.getVertexAttrib(textureCoord, gl.VERTEX_ATTRIB_ARRAY_ENABLED);

              bindQuadProgram();
              fn();

              gl.useProgram(prevProgram);
              gl.activeTexture(prevActiveTexture);
              gl.bindTexture(gl.TEXTURE_2D, prevTexture);
              gl.bindBuffer(gl.ARRAY_BUFFER, prevArrayBuffer);
              if (!prevVertexAttribPosition) gl.disableVertexAttribArray(vertexPosition);
              if (!prevVertexAttribTextureCoord) gl.disableVertexAttribArray(textureCoord);

              binded = false;
            } else {
              bindQuadProgram();
              fn();
            }
          }

          Module.unbindGL = function() {};
          Module.bind3Dfx = function(width, height) {
            Module.unbindGL();

            if (Module.glfx) {
              // screen viewport
              gl.viewport(0, 0, width, height);

              const fboTexture = gl.createTexture();
              gl.bindTexture(gl.TEXTURE_2D, fboTexture);
              gl.texImage2D(
                  gl.TEXTURE_2D, 0, gl.RGBA, width, height, 0,
                  gl.RGBA, gl.UNSIGNED_BYTE, null
              );
              gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
              gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
              gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
              gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);

              const depthStencilBuffer = gl.createRenderbuffer();
              gl.bindRenderbuffer(gl.RENDERBUFFER, depthStencilBuffer);
              gl.renderbufferStorage(gl.RENDERBUFFER, gl.DEPTH_STENCIL, width, height);

              const fbo = gl.createFramebuffer();
              gl.bindFramebuffer(gl.FRAMEBUFFER, fbo);

              gl.framebufferTexture2D(
                gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, fboTexture, 0
              );

              gl.framebufferRenderbuffer(
                gl.FRAMEBUFFER,
                gl.DEPTH_STENCIL_ATTACHMENT,
                gl.RENDERBUFFER,
                depthStencilBuffer
              );

              if (gl.checkFramebufferStatus(gl.FRAMEBUFFER) !== gl.FRAMEBUFFER_COMPLETE) {
                console.error("Framebuffer is not complete");
              }

              gl.bindFramebuffer(gl.FRAMEBUFFER, fbo);

              Module.fboTexture = fboTexture;
              Module.fbo = fbo;
              Module.unbindGL = function() {
                gl.bindFramebuffer(gl.FRAMEBUFFER, null);
                gl.deleteTexture(fboTexture);
                gl.deleteRenderbuffer(depthStencilBuffer);
                gl.deleteFramebuffer(fbo);
              };

              Module.swapbuffers = function() {
                gl.bindFramebuffer(gl.FRAMEBUFFER, null);
                withQuadProgram(function() {
                  gl.bindTexture(gl.TEXTURE_2D, fboTexture);
                  gl.drawArrays(gl.TRIANGLES, 0, 6);
                }, true);
                gl.bindFramebuffer(gl.FRAMEBUFFER, fbo);
              };
            } else {
              Module.swapbuffers = function() {
                console.error("swapbuffers called but glfx is not enabled");
              };
              Module.unbindGL = function() {};
            }

            Module.gl.viewport(0, 0, width, height);
          };

          let requestAnimationFrameId = null;
          Module.updateTexture = (frame, frameWidth, frameHeight) => {
            if (requestAnimationFrameId === null) {
              requestAnimationFrameId = requestAnimationFrame(() => {
                withQuadProgram(function() {
                  gl.bindTexture(gl.TEXTURE_2D, screenTexture);
                  gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGB,
                    frameWidth, frameHeight, 0, gl.RGB, gl.UNSIGNED_BYTE,
                    Module.HEAPU8.slice(frame, frame + frameWidth * frameHeight * 3));
                  gl.drawArrays(gl.TRIANGLES, 0, 6);
                }, false);

                if (Module.glfx) {
                  Module.swapbuffers();
                }
                requestAnimationFrameId = null;
              });
            }
          };
        })();
      } catch (e) {
        Module.err(e.message);
      }
    }

    sendMessage("ws-ready", {});
  });

EM_JS(void, emsc_ws_client_frame_set_size, (int width, int height), {
    if (Module.canvas) {
      if (Module.worker) {
        self.screen.width = width;
        self.screen.height = height;
      }
      Module.canvas.width = width;
      Module.canvas.height = height;

      Module.bind3Dfx(width, height);
    }

    Module.sendMessage("ws-frame-set-size", {width : width, height : height});
  });

EM_JS(void, emsc_start_frame_update, (), {
    Module.frame_update_lines = [];
    Module.frame_update_lines_transferable = [];
  });

EM_JS(void, emsc_add_frame_line, (uint32_t start, uint8_t* ptr, uint32_t len), {
    if (Module.canvas) {
      Module.frame_update_lines.push({});
    } else {
      var bpp3 = Module.HEAPU8.slice(ptr, ptr + len);
      Module.frame_update_lines.push({start : start, heapu8 : bpp3});
      Module.frame_update_lines_transferable.push(bpp3.buffer);
    }
  });

EM_JS(void, emsc_end_frame_update, (uint8_t* frameRgb, uint32_t frameWidth, uint32_t frameHeight), {
    if (Module.frame_update_lines.length > 0) {
      ++Module.messageFrame;
      if (Module.canvas) {
        if (frameWidth > 0 && frameHeight > 0) {
          Module.updateTexture(frameRgb, frameWidth, frameHeight);
        }
      } else {
        Module.sendMessage("ws-update-lines",
          { lines: Module.frame_update_lines },
          Module.frame_update_lines_transferable);
      }
    }
    delete Module.frame_update_lines;
    delete Module.frame_update_lines_transferable;
  });

EM_JS(void, emsc_ws_client_sound_init, (int freq), {
    if (Module.audioPort) {
      Module.sendMessage("ws-sound-init", { freq : 0 });
    } else {
      Module.sendMessage("ws-sound-init", { freq : freq });
    }
  });

EM_JS(void, emsc_ws_client_sound_push, (const float *samples, int num_samples), {
    if (num_samples <= 0) {
        return;
    }

    ++Module.messageSound;
    const heapf32 = Module.HEAPF32.slice(samples / 4, samples / 4 + num_samples);
    if (Module.audioPort) {
      Module.audioPort.postMessage(heapf32, [heapf32.buffer]);
    } else {
      Module.sendMessage("ws-sound-push",
        { samples: heapf32 },
        [ heapf32.buffer ]);
    }
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

      if (!(bytes[0] === 0x50 && bytes[1] === 0x4b)) {
        traverseSockdriveChanges(bytes, (url, changes) => {
          Module.sockdriveChanges[url] = changes;
        });
        continue;
      }

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
          // ignore
        }

        Module.fsCreatedAt = Module._get_changes_mtime_ms();

        // delay needed to create rest of files after fsCreatedAt
        const nowTime = Date.now();
        while (Date.now() < nowTime + 4) {
          // wait
        }
      }
    }

    if (dosboxConf === null) {
      try {
        dosboxConf = Module.FS.readFile("/home/web_user/.jsdos/dosbox.conf");
      } catch (e) {
        Module.err("Broken bundle, .jsdos/dosbox.conf not found");
        return;
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
  if (frameRgb) {
    delete[] frameRgb;
  }
  frameHeight = height;
  frameWidth = width;
  frameRgb = new uint8_t[width * height * 3];
  emsc_ws_client_frame_set_size(width, height);
}

void client_frame_update_lines(uint32_t *lines, uint32_t batchCount, void *rgba, bool bgra) {
  if (!frameRgb) {
    return;
  }

  uint8_t r = 0;
  uint8_t b = 2;
  if (bgra) {
    r = 2;
    b = 0;
  }

  emsc_start_frame_update();
  for (uint32_t i = 0; i < batchCount; ++i) {
    uint32_t base = i * 3;
    uint32_t start = lines[base];
    uint32_t count = lines[base + 1];
    uint32_t offset = lines[base + 2];

    uint8_t* bpp3Begin = frameRgb + start * frameWidth * 3;
    uint8_t* bpp3 = bpp3Begin;
    uint8_t* bpp4 = (uint8_t*) rgba + offset;
    uint8_t* bpp4End = bpp4 + sizeof(uint32_t) * count * frameWidth;
    while (bpp4 < bpp4End) {
      bpp3[0] = bpp4[r];
      bpp3[1] = bpp4[1];
      bpp3[2] = bpp4[b];
      bpp3 += 3;
      bpp4 += 4;
    }
    emsc_add_frame_line(start, bpp3Begin, 3 * count * frameWidth);
  }
  emsc_end_frame_update(frameRgb, frameWidth, frameHeight);
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

EM_ASYNC_JS(void, em_unload, (), {
  return new Promise((resolve) => {
    Module.sendMessage("ws-unload");
    Module.wsUnloadResolve = () => {
      delete Module.wsUnloadResolve;
      resolve();
    };
  });
});

EM_JS(bool, em_net_send, (uint32_t peerId, const void *datap, int len), {
  const data = Module.HEAPU8.slice(datap, datap + len);
  Module.sendMessage("ws-net-send", { peerId, data  }, [ data.buffer ]);
  return true;
});

EM_JS(void, em_net_disconnect, (uint32_t peerId), {
  Module.sendMessage("ws-net-disconnect", { peerId });
});

extern "C" void EMSCRIPTEN_KEEPALIVE em_client_sockdrive_opened(
  uint32_t handle, uint32_t size, uint32_t heads, uint32_t cylinders, uint32_t sectors,
  uint32_t sectorSize, uint32_t aheadRange, uint32_t emptyRangesCount,
  uint8_t* emptyRanges) {

#ifdef JSDOS_X
  client_sockdrive_opened(handle, size, heads, cylinders, sectors, sectorSize, aheadRange, emptyRangesCount, emptyRanges);
#endif
}

extern "C" void EMSCRIPTEN_KEEPALIVE em_client_sockdrive_new_range(
  uint32_t handle, uint32_t range, uint8_t* buffer) {
#ifdef JSDOS_X
  client_sockdrive_new_range(handle, range, buffer);
#endif
}

EM_JS(void, em_server_sockdrive_open, (uint32_t handle, const char* url), {
  url = UTF8ToString(url)
    .replace("wss://sockdrive.js-dos.com:8001/dos.zone/",
        "https://br.cdn.dos.zone/sockdrive-qcow2/dos.zone-")
    .replace("wss://sockdrive.js-dos.com:8001/system/",
        "https://br.cdn.dos.zone/sockdrive-qcow2/system-");

  if (url.endsWith("/")) {
      url = url.slice(0, -1);
  }

  sockdrive(url, Module.sockdriveChanges[url], Module.sockdrivePreload, (range, buffer) => {
      Module.onSockdriveNewRange(handle, range, buffer);
  }).then((drive) => {
      Module.sockdrives[handle] = drive;
      delete Module.sockdriveChanges[url];
      const emptyRanges = Array.from(drive.info.dropped_ranges);
      Module.onSockdriveOpened(
          handle,
          drive.info.size,
          drive.info.heads,
          drive.info.cylinders,
          drive.info.sectors,
          drive.info.sector_size,
          drive.info.ahead_read,
          drive.info.dropped_ranges.length,
          emptyRanges,
      );
  }).catch((e) => {
      Module.err("Can't open sockdrive(" + url + "): " + e.message);
      console.error(e);

      Module.onSockdriveOpened(
          handle,
          0,
          0,
          0,
          0,
          0,
          0,
          0,
          [],
      );
  });
});

EM_JS(void, em_server_sockdrive_ready, (uint32_t handle), {
  Module.sockdrives[handle].ready();
});

EM_JS(void, em_server_sockdrive_close, (uint32_t handle), {
  delete Module.sockdrives[handle];
});

EM_JS(void, em_server_sockdrive_load_range, (uint32_t handle, uint32_t range), {
  Module.sockdrives[handle].readRangeAsync(range);
});

EM_JS(void, em_server_sockdrive_write_sector, (uint32_t handle, uint32_t sector, uint8_t* buffer), {
  const data = HEAPU8.slice(buffer, buffer + Module.sockdriveSectorSize);
  Module.sockdrives[handle].write(sector, data);
});

void server_sockdrive_open(uint32_t handle, const char* address) {
  em_server_sockdrive_open(handle, address);
}
void server_sockdrive_ready(uint32_t handle) {
  em_server_sockdrive_ready(handle);
}
void server_sockdrive_close(uint32_t handle) {
  em_server_sockdrive_close(handle);
}
void server_sockdrive_load_range(uint32_t handle, uint32_t origin) {
  em_server_sockdrive_load_range(handle, origin);
}
void server_sockdrive_write_sector(uint32_t handle, uint32_t sector, uint8_t* buffer) {
  em_server_sockdrive_write_sector(handle, sector, buffer);
}

int server_net_send(uint32_t peerId, const void *datap, int len) {
  return em_net_send(peerId, datap, len) ? len : -1;
}

extern "C" void EMSCRIPTEN_KEEPALIVE ws_client_net_recv(uint32_t peerId, void *datap, int len) {
  client_net_recv(peerId, datap, len);
}

void server_net_disconnect(uint32_t peerId) {
  em_net_disconnect(peerId);
}

void server_unload() {
  em_unload();
}

extern "C" int EMSCRIPTEN_KEEPALIVE fsDeleteFile(const char* path) {
  return std::filesystem::remove_all(path) ? 1 : 0;
}

extern "C" const char* EMSCRIPTEN_KEEPALIVE emGetRunningProgram() {
  return server_get_running_program();
}
