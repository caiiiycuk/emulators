# Project Rules

These instructions apply to the whole repository.

## Code Style

- Keep changes consistent with the existing TypeScript and test style.
- Do not introduce unrelated refactors or formatting churn.
- Before handing off source or test changes, lint both source and tests:

```bash
yarn run eslint src --ext ts,tsx --max-warnings 0
yarn run eslint test --ext ts,tsx --max-warnings 0
```

- Style-only issues may be fixed with eslint autofix:

```bash
yarn run eslint src --ext ts,tsx --max-warnings 0 --fix
yarn run eslint test --ext ts,tsx --max-warnings 0 --fix
```

## Required Verification

- After source, test, or build-related changes, `yarn test:node:build` must pass.
- For very large changes, also run:

```bash
yarn test:browser
yarn test:browser:net
```

- `yarn test:browser:net` may be flaky. If it fails, report the failure as a warning with the relevant error output; do not treat that failure alone as a blocking result.
