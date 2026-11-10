// @ts-check
import eslint from '@eslint/js'
import furystack from '@furystack/eslint-plugin'
import prettierConfig from 'eslint-config-prettier'
import { defineConfig } from 'eslint/config'
import tseslint from 'typescript-eslint'

export default defineConfig(
  {
    ignores: ['dist/**', 'node_modules/**', 'src/cos-sdk/**', 'prettier.config.js', 'eslint.config.js'],
  },
  eslint.configs.recommended,
  ...tseslint.configs.recommendedTypeChecked,
  prettierConfig,
  {
    linterOptions: {
      reportUnusedDisableDirectives: true,
    },
    languageOptions: {
      parserOptions: {
        projectService: {
          allowDefaultProject: ['*.config.*', '*.config.ts', 'tools/*.ts', 'tools/*.test.ts'],
          defaultProject: 'tsconfig.json',
        },
        tsconfigRootDir: import.meta.dirname,
      },
    },
    rules: {
      '@typescript-eslint/no-unused-vars': 'off', // Use TypeScript's own check
      '@typescript-eslint/no-explicit-any': 'off',
      '@typescript-eslint/consistent-type-imports': 'error',
      '@typescript-eslint/array-type': ['error', { default: 'array-simple', readonly: 'array-simple' }],
      '@typescript-eslint/no-useless-constructor': 'error',
      '@typescript-eslint/require-await': 'off',
      '@typescript-eslint/no-unsafe-assignment': 'error',
      'object-shorthand': 'error',
      'dot-notation': 'error',
      'no-caller': 'error',
      'no-useless-concat': 'error',
      radix: 'error',
      yoda: 'error',
      'prefer-arrow-callback': 'error',
      'prefer-rest-params': 'error',
      'no-var': 'error',
      'prefer-const': 'error',
      'prefer-spread': 'error',
      'no-shadow': 'error',
      'prefer-template': 'error',
      'prefer-destructuring': ['error', { array: false, object: true }],
      'default-case': 'error',
    },
  },
  {
    files: ['**/*.spec.ts', '**/*.spec.tsx', '**/*.test.ts', '**/*.test.tsx'],
    rules: {
      '@typescript-eslint/unbound-method': 'off', // vi.fn() is fine in tests
    },
  },
  {
    files: ['tools/**/*.ts', '*.config.ts', '*.config.js', '*.config.mjs'],
    ...tseslint.configs.disableTypeChecked,
  },
  {
    files: ['tools/**/*.ts', '*.config.ts', '*.config.js', '*.config.mjs'],
    rules: {
      '@typescript-eslint/consistent-type-imports': 'off',
    },
  },
  {
    plugins: {
      furystack,
    },
    ...furystack.configs.recommendedStrict,
  },
  {
    files: ['src/**/*.ts', 'src/**/*.tsx'],
    ...furystack.configs.shadesStrict,
  },
)
