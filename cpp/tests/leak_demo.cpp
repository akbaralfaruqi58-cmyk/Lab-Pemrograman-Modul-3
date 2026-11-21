// Lab 2.4 Defect B: intentionally compare a raw owning pointer with RAII.
// Run as `leak_demo raw` to reproduce the leak, or `leak_demo raii` to show
// deterministic cleanup. The normal test runner expects raw mode to fail.

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

class LoanResource {
public:
    LoanResource() { ++live_; }
    ~LoanResource() { --live_; }

    LoanResource(const LoanResource&) = delete;
    LoanResource& operator=(const LoanResource&) = delete;

    [[nodiscard]] static int liveCount() noexcept { return live_; }

private:
    static int live_;
};

int LoanResource::live_ = 0;

static void rawPointerErrorPath() {
    LoanResource* loan = new LoanResource();
    (void)loan;  // ownership is lost when the exception unwinds this frame
    throw std::runtime_error("simulated failure after raw allocation");
}

static void uniquePointerErrorPath() {
    auto loan = std::make_unique<LoanResource>();
    (void)loan;
    throw std::runtime_error("simulated failure after RAII allocation");
}

int main(int argc, char** argv) {
    const std::string mode = argc > 1 ? argv[1] : "raii";
    const bool raw = mode == "raw";
    if (!raw && mode != "raii") {
        std::cerr << "usage: leak_demo [raw|raii]\n";
        return 2;
    }

    try {
        if (raw) rawPointerErrorPath();
        else uniquePointerErrorPath();
    } catch (const std::exception& error) {
        std::cerr << "caught: " << error.what() << '\n';
    }

    std::cout << "live objects after exception: "
              << LoanResource::liveCount() << '\n';

    // raw mode intentionally returns non-zero; raii mode must return zero.
    if (raw) return LoanResource::liveCount() == 1 ? 1 : 0;
    return LoanResource::liveCount() == 0 ? 0 : 1;
}
