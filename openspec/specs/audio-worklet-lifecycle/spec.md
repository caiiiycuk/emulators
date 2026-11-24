# Native AudioWorklet Lifecycle

## Purpose

Release SDK-owned audio resources after emulator stop or failed startup so recreating the player does not leave running contexts or handlers from the previous instance.

## Requirements

### Requirement: Release audio on stop

When stopped through `CommandInterface.exit()`, the SDK MUST initiate closure of the instance-owned `AudioContext`, disconnect its `AudioWorkletNode`, close its available owned `MessagePort`, and remove gesture handlers installed to resume audio. After asynchronous closure completes, the context SHALL be in the `closed` state. This behavior MUST apply to both direct and worker backends.

#### Scenario: Stop a running instance
- **WHEN** a successfully started direct or worker backend with native worklet is stopped through `CommandInterface.exit()`
- **THEN** its context becomes `closed`, its node is disconnected, its audio channel is released, and its previous `pointerdown` and `keydown` handlers are removed

#### Scenario: Stop before the first user gesture
- **WHEN** an instance is stopped with its context in the `suspended` state
- **THEN** the context is also closed, and a subsequent user gesture does not resume audio for the stopped instance

### Requirement: Release resources on failed initialization

The SDK MUST release already-created audio resources when worklet creation or backend initialization fails. The backend error SHALL remain available to the caller, and the existing fallback behavior when worklet is unavailable MUST be preserved.

#### Scenario: Worklet loading or node creation fails
- **WHEN** loading the worklet module or creating the node fails after context creation
- **THEN** the created context is closed, partially created resources are released, and the SDK preserves its existing fallback without worklet

#### Scenario: Backend initialization fails
- **WHEN** direct backend initialization or both worker backend installation attempts fail after audio creation
- **THEN** audio resources are released, and the caller receives the initialization error instead of a successful result

### Requirement: Error messages do not stop audio

A `ws-err` message in a direct or worker backend or a `Worker.onerror` event alone MUST NOT close audio, terminate the transport, or interrupt asynchronous backend creation. The existing error message channel MUST be preserved. Cleanup on actual backend termination and initialization rejection SHALL be preserved.

#### Scenario: A running backend reports ws-err
- **WHEN** a running direct or worker backend reports `ws-err`
- **THEN** the audio context and node remain active, the port and audio resume handlers are preserved, and the message is passed to the existing handler

#### Scenario: Worker error during audio creation
- **WHEN** a worker error occurs before asynchronous worklet loading completes
- **THEN** the error alone does not interrupt worker installation, and created audio resources are preserved until subsequent backend termination

#### Scenario: Error in a running worker
- **WHEN** a running worker with created audio receives a `Worker.onerror` event
- **THEN** audio resources and transport are preserved, and subsequent `CommandInterface.exit()` waits for `ws-exit` and releases audio

### Requirement: Repeated cleanup and restart are safe

Audio resource cleanup MUST be idempotent. A new instance MUST own its resources independently of previously stopped instances.

#### Scenario: Repeated termination
- **WHEN** cleanup of one instance is called again after normal stop or failure
- **THEN** repeated cleanup throws no exceptions and does not close the context, port, or node again

#### Scenario: Create, stop, and recreate cycle
- **WHEN** a browser player with native worklet is created, stopped, recreated, and stopped again
- **THEN** all contexts of that instance are closed after each stop, and only the new instance's context is active during the second run

### Requirement: Compatibility and fix delivery

The SDK MUST preserve the public `CommandInterface`, `BackendOptions`, existing native worklet enablement, and audio processing algorithm. The fix MUST be included in the standard SDK production package and require no dependency file modifications or JavaScript substitution by the consumer.

#### Scenario: Use the standard package
- **WHEN** a consumer installs the released fixed SDK version and starts DOSBox or DOSBox-X with native worklet
- **THEN** the lifecycle fix works without patch files, test overrides, or disabling worklet in the consumer

#### Scenario: Verify backward compatibility
- **WHEN** existing protocol, audio, and backend checks run without changing client code
- **THEN** public signatures and the message format are preserved, and checks pass without changing expected DSP behavior
