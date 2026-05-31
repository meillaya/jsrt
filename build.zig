const std = @import("std");
const builtin = @import("builtin");

comptime {
    if (builtin.zig_version.order(.{ .major = 0, .minor = 16, .patch = 0 }) == .lt) {
        @compileError("jsrt requires Zig 0.16.0 or newer");
    }
}

const runtime_sources = &.{
    "src/memory.c",
    "src/diagnostics.c",
    "src/lexer.c",
    "src/ast.c",
    "src/parser.c",
    "src/value.c",
    "src/env.c",
    "src/gc.c",
    "src/eval.c",
    "src/runtime.c",
    "src/bytecode.c",
    "src/compiler.c",
    "src/vm.c",
};

const cli_sources = &.{
    "src/main.c",
};

const test_sources = &.{
    "tests/test_runner.c",
    "tests/test_lexer.c",
    "tests/test_parser.c",
    "tests/test_eval.c",
    "tests/test_runtime.c",
    "tests/test_diagnostics.c",
    "tests/test_bytecode.c",
    "tests/test_vm.c",
};

const base_c_flags = &.{
    "-std=c17",
    "-Wall",
    "-Wextra",
    "-Werror",
    "-pedantic",
};

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const iterations = b.option([]const u8, "iterations", "Benchmark iterations") orelse "5";
    const warmups = b.option([]const u8, "warmups", "Benchmark warmups") orelse "1";
    const fixture = b.option([]const u8, "fixture", "Benchmark fixture filter path") orelse "";
    const filter = b.option([]const u8, "filter", "Benchmark text filter") orelse "";
    const asan_cc = b.option([]const u8, "asan-cc", "Host C compiler for the AddressSanitizer lane") orelse "cc";

    const jsrt = addJsrtExecutable(b, target, optimize, .{});
    const test_jsrt = addTestExecutable(b, target, optimize, .{});
    b.installArtifact(jsrt);
    b.installArtifact(test_jsrt);

    const sync_default = addSyncStep(b, jsrt, test_jsrt, "sync build artifacts");
    b.default_step.dependOn(sync_default);

    const all_step = b.step("all", "Build jsrt and test_jsrt into ./build");
    all_step.dependOn(sync_default);

    const debug_jsrt = addJsrtExecutable(b, target, .Debug, .{ .debug_macro = true, .suffix = "_debug" });
    const debug_test = addTestExecutable(b, target, .Debug, .{ .debug_macro = true, .suffix = "_debug" });
    const debug_sync = addSyncStep(b, debug_jsrt, debug_test, "sync debug build artifacts");
    const debug_step = b.step("debug", "Build debug jsrt artifacts into ./build");
    debug_step.dependOn(debug_sync);

    const test_step = b.step("test", "Build and run unit tests");
    const run_tests = b.addSystemCommand(&.{"./build/test_jsrt"});
    run_tests.step.dependOn(sync_default);
    test_step.dependOn(&run_tests.step);

    const conformance_step = b.step("conformance", "Run repo conformance harness");
    const run_conformance = addScriptRun(b, "conformance", &.{ "bash", "scripts/conformance.sh" });
    run_conformance.setEnvironmentVariable("BIN", "./build/jsrt");
    run_conformance.step.dependOn(sync_default);
    conformance_step.dependOn(&run_conformance.step);

    const test262_step = b.step("test262-adapter", "Validate Test262 subset manifest");
    const run_test262 = addScriptRun(b, "test262-adapter", &.{ "bash", "scripts/test262-adapter.sh" });
    test262_step.dependOn(&run_test262.step);

    const vm_parity_step = b.step("vm-parity", "Run tree/VM parity harness");
    const run_vm_parity = addScriptRun(b, "vm-parity", &.{ "bash", "scripts/vm-parity.sh" });
    run_vm_parity.setEnvironmentVariable("BIN", "./build/jsrt");
    run_vm_parity.step.dependOn(sync_default);
    vm_parity_step.dependOn(&run_vm_parity.step);

    const verify_step = b.step("verify-production", "Run production verification gates");
    verify_step.dependOn(test_step);
    verify_step.dependOn(conformance_step);
    verify_step.dependOn(test262_step);
    verify_step.dependOn(vm_parity_step);

    addBenchStep(b, "bench", "bench", iterations, warmups, fixture, filter, sync_default);
    addBenchStep(b, "bench-vm", "bench-vm", iterations, warmups, fixture, filter, sync_default);
    addBenchStep(b, "bench-compare", "bench-compare", iterations, warmups, fixture, filter, sync_default);
    addBenchStep(b, "bench-smoke", "bench-smoke", "1", "0", fixture, filter, sync_default);

    const asan_step = b.step("asan", "Build with host AddressSanitizer flags and run unit tests");
    const asan_mkdir = b.addSystemCommand(&.{ "mkdir", "-p", "build" });
    const build_asan_jsrt = addHostAsanCompile(b, asan_cc, cli_sources, "build/jsrt");
    build_asan_jsrt.step.dependOn(&asan_mkdir.step);
    const build_asan_test = addHostAsanCompile(b, asan_cc, test_sources, "build/test_jsrt");
    build_asan_test.step.dependOn(&asan_mkdir.step);
    const run_asan_tests = b.addSystemCommand(&.{"./build/test_jsrt"});
    run_asan_tests.step.dependOn(&build_asan_jsrt.step);
    run_asan_tests.step.dependOn(&build_asan_test.step);
    asan_step.dependOn(&run_asan_tests.step);

    const clean_step = b.step("clean", "Remove generated build artifacts");
    const clean = b.addSystemCommand(&.{ "rm", "-rf", "build", "zig-out" });
    clean_step.dependOn(&clean.step);
}

const ArtifactOptions = struct {
    debug_macro: bool = false,
    suffix: []const u8 = "",
};

fn addJsrtExecutable(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    options: ArtifactOptions,
) *std.Build.Step.Compile {
    const exe = b.addExecutable(.{
        .name = b.fmt("jsrt{s}", .{options.suffix}),
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    addCommonC(exe, b, options);
    exe.root_module.addCSourceFiles(.{ .files = cli_sources, .flags = base_c_flags });
    return exe;
}

fn addTestExecutable(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    options: ArtifactOptions,
) *std.Build.Step.Compile {
    const exe = b.addExecutable(.{
        .name = b.fmt("test_jsrt{s}", .{options.suffix}),
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    addCommonC(exe, b, options);
    exe.root_module.addCSourceFiles(.{ .files = test_sources, .flags = base_c_flags });
    return exe;
}

fn addCommonC(exe: *std.Build.Step.Compile, b: *std.Build, options: ArtifactOptions) void {
    exe.root_module.addIncludePath(b.path("include"));
    if (options.debug_macro) {
        exe.root_module.addCMacro("JSRT_DEBUG", "1");
    }
    exe.root_module.addCSourceFiles(.{ .files = runtime_sources, .flags = base_c_flags });
}

fn addHostAsanCompile(
    b: *std.Build,
    cc: []const u8,
    extra_sources: []const []const u8,
    output: []const u8,
) *std.Build.Step.Run {
    const run = b.addSystemCommand(&.{cc});
    run.addArgs(&.{
        "-std=c17",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-pedantic",
        "-Iinclude",
        "-g",
        "-fsanitize=address",
        "-fno-omit-frame-pointer",
    });
    inline for (runtime_sources) |src| run.addArg(src);
    for (extra_sources) |src| run.addArg(src);
    run.addArgs(&.{ "-o", output });
    return run;
}

fn addSyncStep(
    b: *std.Build,
    jsrt: *std.Build.Step.Compile,
    test_jsrt: *std.Build.Step.Compile,
    name: []const u8,
) *std.Build.Step {
    const step = b.step(name, "Copy built executables to legacy ./build paths");
    const mkdir = b.addSystemCommand(&.{ "mkdir", "-p", "build" });

    const copy_jsrt = b.addSystemCommand(&.{"cp"});
    copy_jsrt.step.dependOn(&mkdir.step);
    copy_jsrt.addFileArg(jsrt.getEmittedBin());
    copy_jsrt.addArg("build/jsrt");

    const copy_test = b.addSystemCommand(&.{"cp"});
    copy_test.step.dependOn(&mkdir.step);
    copy_test.addFileArg(test_jsrt.getEmittedBin());
    copy_test.addArg("build/test_jsrt");

    const chmod = b.addSystemCommand(&.{ "chmod", "+x", "build/jsrt", "build/test_jsrt" });
    chmod.step.dependOn(&copy_jsrt.step);
    chmod.step.dependOn(&copy_test.step);

    step.dependOn(&chmod.step);
    return step;
}

fn addScriptRun(b: *std.Build, step_name: []const u8, argv: []const []const u8) *std.Build.Step.Run {
    const run = b.addSystemCommand(argv);
    run.step.name = b.fmt("run {s}", .{step_name});
    return run;
}

fn addBenchStep(
    b: *std.Build,
    step_name: []const u8,
    mode: []const u8,
    iterations: []const u8,
    warmups: []const u8,
    fixture: []const u8,
    filter: []const u8,
    build_step: *std.Build.Step,
) void {
    const step = b.step(step_name, b.fmt("Run benchmark harness mode {s}", .{mode}));
    const run = b.addSystemCommand(&.{ "bash", "scripts/benchmark.sh", mode });
    run.setEnvironmentVariable("BINARY", "./build/jsrt");
    run.setEnvironmentVariable("ITERATIONS", iterations);
    run.setEnvironmentVariable("WARMUPS", warmups);
    if (fixture.len != 0) run.setEnvironmentVariable("FIXTURE", fixture);
    if (filter.len != 0) run.setEnvironmentVariable("FILTER", filter);
    run.step.dependOn(build_step);
    step.dependOn(&run.step);
}
