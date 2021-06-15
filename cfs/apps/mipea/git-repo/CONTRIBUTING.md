# Contributing

Any contributions are welcome. Thanks for reading this file.

## Testing

If you find any bug or have ideas for improvements
please open an issue or pull request.

One of the goals of this library is to run on all Raspberry Pis. The problem
is that I don't own all the different models. If you have successfully tested
mipea on a Pi that is not listed as tested than please inform me.

## Submitting changes

* Please send a [GitHub Pull Request](https://github.com/jasLogic/mipea/pulls)
with a clear explanation of what you have done
* Please follow the coding conventions (below)
* Try to have one feature per commit
* Always write a clear log message for your commits

## Coding conventions

I try to adapt my coding style to the
[Linux kernel coding style](https://www.kernel.org/doc/html/v4.10/process/coding-style.html),
here are the most important aspects:

* Indent with tabs
* Tabs are 8 spaces wide
* Try to have columns no longer than 80 characters
* Curly brackets come onto the same line when opening a block and on the next
line when closing it, for example:
```
if (this) {
    that();
}
```
* Functions have the opening curly bracket on the next line, for example:
```
void that(void)
{
    return;
}
```
* Use [snake case](https://en.wikipedia.org/wiki/Snake_case) with lower case
for naming everything
    * I try to name functions and global variables like this:
    [peripheral]\_[action]\_[sub-action or specification], for example:
    `i2c_write_byte`
