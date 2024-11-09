Practical examples and homeworks for "System Programming" course of lectures.

The course: https://slides.com/gerold103/decks/sysprog and https://slides.com/gerold103/decks/sysprog_eng

## Running the tests

### Manual

The folders with numbers as names contain the homeworks with tests (where they could be written). The students are welcome to build and run the tests manually - they should work in any non-Windows system. Alternatively they can be run in Docker. It is recommended to use the latest ubuntu Docker image for that.

In order to check more than just the functional correctness it is encouraged to also check for the memory leaks using `utils/heap_help/`.

### Local auto-tests

In order to run the tests the same as a remote automatic system would do (like VK All Cups) the participants can use the following commands right in their `sysprog/` folder.
```Bash
# Build the docker image used for running the tests in a stable isolated Linux
# environment.
docker build . -t allcups -f allcups/DockAllcups.dockerfile

# Run tests for the second homework. One can replace N in `HW=N` with any other
# homework number.
docker run  -v ./:/tmp/data --env IS_LOCAL=1 --env HW=2 allcups
```

The argument `HW=N` allows to specify which homework needs to be tested. The `N` must match the directory name where the solution is located. The solutions are expected to be implemented in a fork of this repository, in the same folders where the tasks and their templates are stored.

### Remote auto-tests

However the above commands are slightly different from what the remote automatic system would run. It would rather execute the following lines, right in the `sysprog/` folder as well.
```Bash
# Specify the number of the homework.
echo "{\"hw\": 3}" > allcups/settings.json

# The file will contain the score.
touch allcups/output.json

# The solution needs to be archived as a zip file.
docker run -v ./:/tmp/data -v ./solutions/3.zip:/opt/client/input -v ./allcups/output.json:/opt/results/output.json allcups
```

The remote system uses the vanilla unchanged `sysprog/` repository to get the original tests and `Makefile`s. Then it takes your solution as an archive (can be created using `cd your_solution; zip -r ../solution.zip ./*`), builds it using the original sysprog/ Makefile from the corresponding homework, and then runs it against the original tests. The homework number is determined from the `settings.json` in the first command above. The result is saved into `output.json` and is printed to the console.

It is highly recommended not to change your local test files and Makefiles at all. Otherwise your solution testing is going to diverge from it is running in the auto-tests system.
