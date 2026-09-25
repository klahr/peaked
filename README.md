# Peeked

An alcohol tracker for Sailfish OS that learns how you feel at different
levels and tells you whether another drink is a good idea.

- Blood alcohol in per mille, estimated from height, weight, age and sex, with
  a graph of the evening so far and where it is heading
- When you will be sober, and a limit line in the graph, 0.2 ‰ by default
- Drink presets with a photo, prefilled from Open Food Facts, picked from the
  gallery or taken with the camera
- Asks how you feel every 30 minutes, from a notification or in the app
- Advice on another drink, from how you felt at the level it would take you to
- Cover with the current level, time left until sober, the advice and buttons
  to finish or start a drink

The estimate uses the Watson formula for body water and a Widmark style model
with absorption from the stomach and a fixed burn rate of 0.15 ‰ per hour.
People differ, and food, sleep and tolerance are not taken into account. It is
an estimate only, never use it to decide whether you can drive.

Needs Sailfish OS 5.1 or newer, on aarch64 or armv7hl. The app is sandboxed and
asks for the Internet, Notifications, Pictures, MediaIndexing and Camera
permissions on first start. Internet is only used to search Open Food Facts.

## Building

Building needs the Sailfish SDK, with the project inside the SDK workspace:

```
sfdk -c target=SailfishOS-5.1.0.11-aarch64 build
sfdk -c target=SailfishOS-5.1.0.11-armv7hl build
```

The RPM ends up in `RPMS/`. Clean the build files between targets.

## License

GPLv3, see [LICENSE](LICENSE).

Drink data and photos found through search come from
[Open Food Facts](https://world.openfoodfacts.org/), data under the ODbL and
photos under CC BY-SA.
