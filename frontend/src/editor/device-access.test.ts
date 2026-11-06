import { createInjector } from "@furystack/inject";
import { describe, expect, it } from "vitest";

import { DeviceAccess, createDeviceAccess, deviceAuthorizationKey } from "./device-access";

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

  it("reuses the same Basic header after a new session is created", () => {
    const stored = new Map<string, string>();
    const storage = {
      getItem: (key: string) => stored.get(key) ?? null,
      setItem: (key: string, value: string) => {
        stored.set(key, value);
      },
      removeItem: (key: string) => {
        stored.delete(key);
      },
    };
    const first = createDeviceAccess(storage);
    expect(first.signIn("cos", "secret")).toBeNull();
    first[Symbol.dispose]();

    const second = createDeviceAccess(storage);
    expect(second.authorization.getValue()).toBe(`Basic ${btoa("cos:secret")}`);
    expect(stored.get(deviceAuthorizationKey)).toBe(second.authorization.getValue());
    second[Symbol.dispose]();
  });
});