const std = @import("std");

pub fn build(b: *std.Build) void {
    const exe = b.addExecutable(.{
        .name = "cemu8",
        .root_module = b.createModule(.{
            .target = b.graph.host,
        }),
    });

    exe.root_module.addCSourceFiles(.{
        .files = &.{
            "src/main.c",
            "src/chip8.c",
        },
        .flags = &.{
            "-Iinclude",
            "-I/usr/local/include/snorkel",
        },
    });

    exe.root_module.linkSystemLibrary("c", .{});
    exe.root_module.linkSystemLibrary("raylib", .{
        .preferred_link_mode = .static,
    });
    exe.root_module.linkSystemLibrary("snorkel", .{});
    b.installArtifact(exe);
}
