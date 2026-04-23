import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';
import react from '@astrojs/react';

// Stellarr's site: custom landing at `/`, Starlight-powered manual at `/docs/*`.
export default defineConfig({
  site: 'https://stellarr.org',
  integrations: [
    react(),
    starlight({
      title: 'STELLARR',
      logo: { src: '../assets/logo.svg', replacesTitle: false },
      favicon: '/favicon.ico',
      // GoatCounter — privacy-friendly analytics, no cookies, no consent
      // banner required. Same instance the old Docsify manual used.
      head: [
        {
          tag: 'script',
          attrs: {
            src: 'https://gc.zgo.at/count.js',
            'data-goatcounter': 'https://stellarr.goatcounter.com/count',
            async: true,
          },
        },
      ],
      customCss: [
        './src/styles/starlight-theme.css',
      ],
      social: [
        { icon: 'github', label: 'GitHub', href: 'https://github.com/stellarr-audio/stellarr' },
      ],
      // Two groups visible at the same time, both un-collapsible by default.
      sidebar: [
        {
          label: 'Manual',
          collapsed: false,
          items: [
            { label: 'Introduction',             slug: 'docs/index' },
            { label: 'Quick Start',              slug: 'docs/quick-start' },
            { label: 'The Grid',                 slug: 'docs/grid' },
            { label: 'Block Options',            slug: 'docs/block-options' },
            { label: 'MIDI',                     slug: 'docs/midi' },
            { label: 'Tuner',                    slug: 'docs/tuner' },
            { label: 'Presets, Scenes & States', slug: 'docs/presets-scenes-states' },
            { label: 'System',                   slug: 'docs/system' },
            { label: 'Privacy & Telemetry',      slug: 'docs/privacy' },
          ],
        },
        {
          label: 'Testing',
          collapsed: false,
          items: [
            { label: 'Overview',           slug: 'docs/testing/index' },
            { label: 'Preset Switching',   slug: 'docs/testing/01-preset-switching' },
            { label: 'Audio Devices',      slug: 'docs/testing/02-audio-devices' },
            { label: 'Plugin Management',  slug: 'docs/testing/03-plugin-management' },
            { label: 'Grid and Routing',   slug: 'docs/testing/04-grid-routing' },
            { label: 'Scenes',             slug: 'docs/testing/05-scenes' },
            { label: 'Tuner',              slug: 'docs/testing/06-tuner' },
            { label: 'MIDI',               slug: 'docs/testing/07-midi' },
            { label: 'Settings',           slug: 'docs/testing/08-settings' },
            { label: 'Loudness Metering',  slug: 'docs/testing/09-loudness-metering' },
          ],
        },
      ],
    }),
  ],
});
