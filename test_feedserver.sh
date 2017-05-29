#!/bin/bash

cd testing
R_LIBS_USER=/tmp R --no-save < test-feedserver.R > test-feedserver.Rout
diff test-feedserver.Rout test-feedserver.RoutSave
