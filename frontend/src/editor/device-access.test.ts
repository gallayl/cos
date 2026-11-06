import { createInjector } from "@furystack/inject";
import { describe, expect, it } from "vitest";

import { DeviceAccess } from "./device-access";

describe("device access", () => {
  it("stores a Basic header and clears it when the device rejects it", async () => {
    const injector = createInjector();
    const access = injector.get(DeviceAccess);

    expect(access.signIn("", "secret")).toBe("Enter the device username");
    expect(access.authorization.getValue()).toBe("");

    expect(access.signIn("cos", "secret")).toBeNull();
    expect(access.authorization.getValue()).toBe(`Basic ${btoa("cos:secret")}`);
    expect(access.needsAuth.getValue()).toBe(false);

    access.markUnauthorized();
    expect(access.needsAuth.getValue()).toBe(true);
    access.signIn("cos", "other");
    expect(access.needsAuth.getValue()).toBe(false);

    await injector[Symbol.asyncDispose]();
  });
});