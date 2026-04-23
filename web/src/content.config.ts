import { defineCollection } from 'astro:content';
import { glob } from 'astro/loaders';
import { docsSchema } from '@astrojs/starlight/schema';

// Source of truth for all docs lives at repo root under `docs/` —
// dev-friendly location. The Astro site loads both `docs/manual/` and
// `docs/testing/` into the `docs` content collection, with slug prefixes
// that preserve the canonical `/docs/*` URL shape.
export const collections = {
  docs: defineCollection({
    loader: glob({
      pattern: '{manual,testing}/**/*.md',
      base: '../docs',
      generateId: ({ entry }) => {
        const stripped = entry.replace(/\.md$/, '');
        // `manual/foo` → `docs/foo`
        // `testing/foo` → `docs/testing/foo`
        if (stripped.startsWith('manual/')) {
          return 'docs/' + stripped.slice('manual/'.length);
        }
        return 'docs/' + stripped;
      },
    }),
    schema: docsSchema(),
  }),
};
