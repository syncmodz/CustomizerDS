# Keeping the LED on

The LED control in the app is live (it talks to the MCU). There are three
levels of "staying on":

1. **While the app runs** — always works.
2. **After you close the app** — the app no longer clears the LED on exit, so
   the pattern stays until a reboot or a system notification.
3. **After a reboot** — needs a Luma patch (below).

## Surviving a reboot

This uses a Luma patch on the notification module `0004013000003502`, applied
on every boot. LED is MCU, not NAND, so it can't brick anything — a bad patch
at worst stops the boot, and you fix it by deleting the file.

In Luma's config (hold SELECT at boot), turn on:

- **Enable game patching**
- **Enable loading external FIRMs and modules**

Then use **CtrRGBPAT2** (or CustomRGBPattern) to generate the patch — it knows
the correct offsets per firmware version. The patch goes to
`/luma/titles/0004013000003502/code.ips`. Reboot to apply.

The app doesn't generate this patch because the offset depends on the firmware
version; a wrong one would just fail to boot until you delete it. To undo,
delete `/luma/titles/0004013000003502/code.ips` and reboot.

References: CtrRGBPAT2 (Golem642), CustomRGBPattern, Luma3DS game patching.
