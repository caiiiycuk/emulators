//
// Created by caiiiycuk on 27.02.2020.
//

#include <jsdos-timer.h>
#include <jsdos-net.h>
#include <protocol.h>
#include <timer.h>

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "../sokol-lib/sokol_app.h"
#include "../sokol-lib/sokol_audio.h"
#ifndef GL4ES
#include "../sokol-lib/sokol_gfx.h"
#include "shaders.h"
#endif
#include <unistd.h>

#include "../sokol-lib/sokol_log.h"
#ifdef USE_HUMBLENET
#include "humblenet_p2p.h"
#endif

#ifdef JSDOS_X
#include <SDL.h>
#endif

std::mutex mutex;

extern void client_run();

int renderedFrame = 0;
int frameCount = 0;

extern int frameWidth = 0;
extern int frameHeight = 0;
extern uint32_t *frameRgba = nullptr;
int frameSize = 0;

static float vertices[] = {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
                           0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};

static float verticesFlipped[] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
                                  1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                                  1.0f, 1.0f, 1.0f, 0.0f};

#ifndef GL4ES
class GfxState {
 public:
  int width;
  int height;
  sg_pass_action pass;
  sg_pipeline pipeline;
  sg_bindings bind;
  sg_shader shader;

  GfxState(int width, int height)
      : width(width), height(height), pass({}), pipeline({}), bind({}) {
    sg_buffer_desc bufferDescription = {};
    bufferDescription.size = sizeof(vertices);
    bufferDescription.data = sg_range{
           .ptr = sg_query_features().origin_top_left ? vertices : verticesFlipped,
           .size = 64,
    };

    bind.vertex_buffers[0] = sg_make_buffer(&bufferDescription);

    sg_pipeline_desc pipelineDescription = {};
    shader = sg_make_shader(display_shader_desc(sg_query_backend()));
    pipelineDescription.shader = shader;
    pipelineDescription.primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP;
    pipelineDescription.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
    pipelineDescription.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;
    pipeline = sg_make_pipeline(&pipelineDescription);

    sg_image_desc imageDescription = {};
    imageDescription.width = width;
    imageDescription.height = height;
    imageDescription.pixel_format = SG_PIXELFORMAT_RGBA8;
    imageDescription.usage = SG_USAGE_STREAM;
    bind.fs.images[0]  = sg_make_image(&imageDescription);

    sg_sampler_desc samplerDescription = {};
    samplerDescription.min_filter = SG_FILTER_LINEAR;
    samplerDescription.mag_filter = SG_FILTER_NEAREST;
    samplerDescription.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
    samplerDescription.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
    bind.fs.samplers[0] = sg_make_sampler(&samplerDescription);
  }

  ~GfxState() {
    sg_destroy_shader(shader);
    sg_destroy_buffer(bind.vertex_buffers[0]);
    sg_destroy_pipeline(pipeline);
    sg_destroy_image(bind.fs.images[0]);
    sg_destroy_sampler(bind.fs.samplers[0]);
  }
};

GfxState *state = nullptr;
#endif


void client_frame_set_size(int width, int height) {
  std::lock_guard<std::mutex> g(mutex);

  if (frameRgba) {
    delete[] frameRgba;
  }
  frameWidth = width;
  frameHeight = height;
  frameSize = width * height;
  frameRgba = new uint32_t[width * height];
}

void client_frame_update_lines(uint32_t *lines, uint32_t count, void *rgba, bool bgra) {
  std::lock_guard<std::mutex> g(mutex);

  if (!frameRgba) {
    return;
  }

  for (uint32_t i = 0; i < count; ++i) {
    uint32_t start = lines[i * 3] * frameWidth;
    uint32_t count = lines[i * 3 + 1] * sizeof(uint32_t) * frameWidth;
    uint32_t offset = lines[i * 3 + 2];
    if (start + count / 4 > frameSize) {
      printf("line is out frame bounds\n");
      continue;
    }
    memcpy(&frameRgba[start], (char *)rgba + offset, count);
    if (bgra) {
        auto begin = (uint8_t*) &frameRgba[start];
        auto end = begin + count;
        while (begin < end) {
            auto r = begin[0];
            begin[0] = begin[2];
            begin[2] = r;
            begin += sizeof(uint32_t);
        }
    }
  }

  frameCount++;
}

void client_stdout(const char* data, uint32_t amount) {
  if (strstr(data, "dhry2:")) {
      printf("%s\n", data);
  }
}

void client_log(const char* tag, const char* message) {
  printf("[%s] %s\n", tag, message);
}

void client_warn(const char* tag, const char* message) {
  printf("WARN! [%s] %s\n", tag, message);
}

void client_error(const char* tag, const char* message) {
  printf("ERR! [%s] %s\n", tag, message);
}

void client_sound_init(int freq) {
  saudio_desc audioDescription = {};
  audioDescription.sample_rate = static_cast<int>(freq);
  audioDescription.num_channels = 1;
  audioDescription.logger.func = slog_func;

  saudio_setup(&audioDescription);
}

void client_sound_push(const float *samples, int num_samples) {
  saudio_push(samples, num_samples);
}

void client_network_connected(NetworkType networkType, const char* address) {
  printf("Network %d connected to %s\n", networkType, address);
}

void client_network_disconnected(NetworkType networkType) {
  printf("Network %d disconnected\n", networkType);
}

void sokolInit() {
#ifndef GL4ES
  sg_desc gfxDescription = {};
  gfxDescription.buffer_pool_size = 4;
  gfxDescription.image_pool_size = 4;
  gfxDescription.shader_pool_size = 4;
  gfxDescription.pipeline_pool_size = 2;
  gfxDescription.context_pool_size = 1;
  gfxDescription.logger.func = slog_func;

  sg_setup(&gfxDescription);
#endif
}

void sokolFrame() {
#ifndef GL4ES
  std::lock_guard<std::mutex> g(mutex);

// @caiiiycuk: think about better solution
//   https://github.com/floooh/sokol/issues/478
//
  if (frameWidth == 0 || frameHeight == 0) {
    return;
  }

  if (!state || state->width != frameWidth || state->height != frameHeight) {
    delete state;

    state = new GfxState(frameWidth, frameHeight);
  }

  sg_image_data imageData = {};
    imageData.subimage[0][0] = {};
    imageData.subimage[0][0].ptr = frameRgba;
    imageData.subimage[0][0].size =
      (state->width) * (state->height) * (int)sizeof(uint32_t);

  sg_update_image(state->bind.fs.images[0], &imageData);

  sg_begin_default_pass(&state->pass, state->width, state->height);
  sg_apply_pipeline(state->pipeline);
  sg_apply_bindings(&state->bind);
  sg_draw(0, 4, 1);
  sg_end_pass();
  sg_commit();

  renderedFrame = frameCount;
#endif
}

void sokolKeyEvent(const sapp_event *event) {
  static std::unordered_map<KBD_KEYS, bool> keyMatrix;
  auto keyCode = (KBD_KEYS) event->key_code;
  auto pressed = event->type == SAPP_EVENTTYPE_KEY_DOWN;
  if (keyMatrix[keyCode] == pressed) {
    return;
  }
  keyMatrix[keyCode] = pressed;
  server_add_key(keyCode, pressed, GetMsPassedFromStart());
}

void runRuntime() {
  std::thread server(server_run);
  client_run();
  server.join();
}

#ifdef USE_HUMBLENET
struct PeerData {
  void* datap;
  int len;
};
bool peerConnectionEstablished = false;
#endif

int main(int argc, char *argv[]) {
#ifdef USE_HUMBLENET
  humblenet_init();
  if (!humblenet_p2p_init("localhost:8080", "sokol", "21e21red", "qweqw2")) {
    printf("Can't init humblenet\n");
    return 1;
  }

  while (humblenet_p2p_get_my_peer_id() == 0) {
    humblenet_p2p_wait(4);
  }

  jsdos::myPeerId = humblenet_p2p_get_my_peer_id();
#endif
  runRuntime();
  return 0;
}

void client_tick() {
#ifdef USE_HUMBLENET
  humblenet_p2p_wait(0);

  static void* buffer = malloc(1024 * 1024);
  PeerId fromPeer;
  int len;
  while ((len = humblenet_p2p_recvfrom(buffer, 1024 * 1024, &fromPeer, 0)) > 0) {
    void* data = malloc(len);
    memcpy(data, buffer, len);
    client_net_recv(fromPeer, data, len);
  }
#endif
}

int server_net_connect(const char* address) {
#ifdef USE_HUMBLENET
    peerConnectionEstablished = false;
    return NETWORK_DOSBOX_IPX;
#else
    return NETWORK_NA;
#endif
}
int server_net_send(uint32_t peerId, const void *datap, int len) {
#ifdef USE_HUMBLENET
    assert(peerId != 0);
    assert(peerId != jsdos::myPeerId);
    int result = humblenet_p2p_sendto(datap, len, peerId, SendMode::SEND_RELIABLE, 0);
    double startedAt = GetMsPassedFromStart();
    while (!peerConnectionEstablished) {
        if (result != 0) {
          peerConnectionEstablished = true;
          break;
        }

        if (GetMsPassedFromStart() - startedAt > 5000) {
          return -1;
        }

        humblenet_p2p_wait(0);
        sleep(1);
        result = humblenet_p2p_sendto(datap, len, peerId, SendMode::SEND_RELIABLE, 0);
    }
    return result;
#else
    return -1;
#endif
}
void server_net_disconnect(uint32_t peerId) {
#ifdef USE_HUMBLENET
  if (peerId != 0) {
    humblenet_p2p_disconnect(peerId);
  }
#endif
}

void server_unload() {
}
