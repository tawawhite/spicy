// Copyright (c) 2020 by the Zeek Project. See LICENSE for details.

#include <hilti/hilti.h>

int main(int argc, char** argv) {
    hilti::Driver driver("hiltic");

    if ( auto rc = driver.parseOptions(argc, argv); ! rc ) {
        hilti::logger().error(rc.error().description());
        exit(1);
    }

    if ( auto rc = driver.run(); ! rc ) {
        hilti::logger().error(rc.error().description());
        exit(1);
    }

    return 0;
}
