
# Coding Guidelines

## General
- Prefer simple, readable solutions.
- Avoid over-engineering.
- Implement only requested features.
- Keep changes small and focused.
- Reuse existing code before adding new abstractions.
- Do not refactor unrelated code.
- Do not add dependencies without a clear reason.

## Embedded C
- Follow the project's existing coding style.
- Preserve existing GPIO and peripheral configurations.
- Use the existing HAL or ESP-IDF APIs.
- Avoid unnecessary tasks, queues, callbacks, and state machines.
- Keep hardware initialization and error handling appropriate to the project.
- Do not block time-sensitive operations.

## Before Finishing
- Review the changes.
- Build or compile when possible.
- Report build errors honestly.
- Explain what changed and why.
