# unbound4j

unbound4j is a high throughput DNS resolution engine for Java application that leverages [libunbound](https://nlnetlabs.nl/documentation/unbound/libunbound/).

It was developed in order to perform reverse lookups at high rates (tens of thousands of lookups per second) while processing network flows.

Building
--------

Requires Maven (tested with 3.5.2) and CMake (tested with 3.14.5)

```sh
./build.sh
```

The dist/ folder should now contain both **unbound4j-VERSION.jar** and **libunbound4j.so**.

