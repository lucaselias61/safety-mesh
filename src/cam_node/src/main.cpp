#include "discovery.hpp"
#include "camera.hpp"
#include "cli/init.hpp"
#include "cli/list.hpp"

#include <CLI/CLI.hpp>

#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>


int main(int argc, char* argv[]) {
    CLI::App app{"RealSense camera node"};
    app.require_subcommand(1);

    list_cmd(app);
    init_cmd(app);

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& error) {
        return app.exit(error);
    } catch (const std::exception& error) {
        std::cerr << "Camera Node command failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
