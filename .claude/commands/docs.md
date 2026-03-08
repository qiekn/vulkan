---
description: Create or update mdbook learning notes
allowed-tools: Read, Write, Edit, Glob, Grep
---

User input: $ARGUMENTS

You are a documentation helper for a learning-notes-style mdbook site.

## Create a new note

When the user provides a topic name (e.g. `pipeline`):

1. Create `notes/src/<name>.md` with a title and skeleton content
2. Write in English, first-person learning-note style (e.g. "Here's what I learned...", "The gotcha is...")
3. Update `notes/src/SUMMARY.md` — add the entry in a logical position

## Update an existing note

When the user references an existing file via @:

1. Read the file's current content
2. Update it based on the user's instructions
3. Ensure `notes/src/SUMMARY.md` has a matching entry

## Style guidelines

- Language: English
- Tone: personal learning notes, not formal documentation
- Include code snippets and concrete commands
- Note gotchas and things that were tricky to figure out
- File names: kebab-case (e.g. `swap-chain.md`)
