# Spec Delta

## Purpose

Deliver errors from the Worker itself (`error` events on the `Worker` object) to the `CommandInterface` consumer as error messages without changing the transport or audio lifecycle.

## ADDED Requirements

### Requirement: Worker errors reach the consumer
A `ws-err` message produced by an `error` event on the `Worker` object MUST include the transport's `sessionId` and a nonempty `tag` so that `CommandInterface` accepts it and forwards it through `events().onMessage("error", "[tag]message")`. The message text SHALL include the event's `message` and, when available, its `filename`.

#### Scenario: Worker reports an error after startup
- **WHEN** the worker backend is running, `CommandInterface` exists, and the `Worker` object emits an `error` event with `message` "boom"
- **THEN** the `events().onMessage` subscriber receives one message of type `error` whose text contains "boom"

#### Scenario: Worker error before the backend is ready
- **WHEN** the `Worker` object emits an `error` event before `ws-server-ready`
- **THEN** the `error` message is delivered to the `events().onMessage` subscriber after subscription through the transport message queue, like any other `ws-*` message sent before `initMessageHandler`

### Requirement: Worker errors do not change the lifecycle
An `error` event on the `Worker` object alone MUST NOT terminate the `Worker`, close audio resources, reject transport creation, or initiate `exit`. These rules from the `audio-worklet-lifecycle` capability remain unchanged.

#### Scenario: Exit after a Worker error
- **WHEN** the consumer calls `ci.exit()` after an `error` event on the `Worker` object and the backend responds with `ws-exit`
- **THEN** `exit` completes, the `Worker` is terminated exactly once, the audio context is closed exactly once, and `onExit` fires once
