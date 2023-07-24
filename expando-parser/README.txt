WIP version of the new expando parser.

It can be built by `make`.
Tests can be run by `make test`.

The tests are checked against a known-to-be-good output. This 'golden
data' can be (re)generated using `./update_tests.sh` then checked by hand.

To add new test:
+ create a new .in file in the tests directory
+ make the expnado string as the *first* line of the file
  (only the first line will be passed to parser)
+ run update_tests.sh
+ verify the generated .checked file
