# Mini Key/Value Store

Find all `TODO`s in the existing code and complete the implementation of the server. In broad strokes, the server binds a client and reads/processes incoming commands (read over a socket) until the client disconnects. The following functions should be supported:

 - `GET x`: Fetches the key x from the backing hashtable and return its value over the socket
 - `SET x y`: Updates the value for key x to be y. Returns 'ok' over the socket on successful completion
 - `CREATE x`: Puts an item with key x in the backing hashtable. Returns 'ok' over the socket on successful completion
 - `DELETE x`: Removes an item with key x from the backing hashtable. Returns 'ok' over the socket on successful completion

## Testing

Use the `telnet` program in an additional shell to connect to the server. For example, to connect to the server running on port 8080 on the local machine, run `telnet localhost 8080`. Refer to `man telnet` for more details.

## Using `git`

Remember to keep your repository history clean, with many small commits and descriptive commit messages. We want to be able to see the progression of your thoughts through the commit log, **not** just a single commit with all your code (and this will be reflected in your grade).

For the most part, the following commands will be all you need:
  - `git add`
  - `git commit`
  - `git status` (*always* use this before committing)
  - `git diff -stat`
  - `git push`
  - `git pull`
  - `git clone`

  You can learn about each of these using the internet, or `git help X` where `X` is the command portion of the above examples.
