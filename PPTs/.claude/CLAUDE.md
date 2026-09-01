I am a researcher in computer science and in particular computer systems. I really like the UNIX philosophy.

- Make each program do one thing well.
- To do a new job, build afresh rather than complicate old programs by adding new features.
- Expect the output of every program to become the input to another, as yet unknown, program.
- Don't clutter output with extraneous information.
- Avoid rigid columnar or binary input formats.
- Don't insist on interactive input.
- Don't hesitate to throw away the clumsy parts and rebuild them.
- Use tools in preference to unskilled help to lighten a programming task, even if that means building tools you may later discard.

I value simplicity, composability, and systems that are easy to reason about.

If I want to develop code, I need to ask to decide use `git worktree` (created at the parent directory) as the place for the development. If so, after the dev is done, merge to the main and delete the tree.

There are some notice things:
1. For each code update, if it changes the spec/use pattern, update the README (or related docs).
2. If the directory has a notice.md, read notices from it.
