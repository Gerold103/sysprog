## SO_REUSEADDR vs SO_REUSEPORT

`SO_REUSEADDR` is an option allowing to 'recycle' an already closed socket, but still
alive in the kernel. The thing is that TCP socket is not deleted from the kernel right
after it is closed, because it still can have some data to send. This is why it happens,
that an attempt to `bind` to an address right after `close` has been called on it, can
lead to an error 'Address in use'. This option allows to immediately delete the closed
socket, if there is an attempt to bind to its address again.

`SO_REUSEPORT` is an option, allowing to load-balance new clients over multiple
processes bound to exactly the same IP and port.

Look at the examples in this folder. There is a server, which tries to bind+listen to
a local address, accept one client, and exit. The client just tries to connect and
exits. The server can accept two command line options: `reuse_addr` and `reuse_port`
to set the corresponding flags before `bind()`.

### First test: try to bind to the same address without any options:
```
#          TERMINAL 1              #          TERMINAL 2              #
$> gcc server.c -o server
$> gcc client.c -o client
$> ./server                        $> ./server
bind success                       bind error: Address already in use
listen success                     $>
^C
```
Failed.

### Second test: try to bind to the same address with `SO_REUSEADDR`:
```
#          TERMINAL 1              #          TERMINAL 2              #
$> ./server reuse_addr             $> ./server reuse_addr
reuse addr is set                  reuse addr is set
bind success                       bind error: Address already in use
listen success                     $>
^C
```
Failed.

### Third test: try to bind to the same address with `SO_REUSEPORT`:
```
#          TERMINAL 1              #          TERMINAL 2              #
$> ./server reuse_port             $> ./server reuse_port
reuse port is set                  reuse port is set
bind success                       bind success
listen success                     listen success
...                                ...
```
It works. Now create a client:

```
#          TERMINAL 1              #          TERMINAL 2              #          TERMINAL 3              #
...                                ...
listen success                     listen success
                                                                      $> ./client
                                   accept success                     connect success
                                   $>                                 $>

                                                                      $> ./client
accept success                                                        connect success
$>                                                                    $>
```
Two clients are accepted by different servers.
