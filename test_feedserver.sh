#!/bin/bash

cd testing
R --no-save < test-feedserver.R > test-feedserver.Rout
diff test-feedserver.Rout test-feedserver.RoutSave
