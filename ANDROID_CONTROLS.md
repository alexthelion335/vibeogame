# Android Touch Controls Layout

```
┌─────────────────────────────────────────────────────────────────────┐
│  [||]                                              [INV]  (Top Bar) │
│  Pause                                             [WPN]             │
│                                                                      │
│                                                                      │
│                    DRAG ANYWHERE HERE                               │
│                    TO ROTATE CAMERA                                 │
│                    (Right 60% of screen)                            │
│                                                                      │
│                                                                      │
│                                                           [JUMP]     │
│  ( • )                                                    [FIRE]     │
│   Joystick                                                (Bottom)   │
└─────────────────────────────────────────────────────────────────────┘
```

## Control Details

### Movement Joystick (Bottom-Left)
- **Position**: 150px from left, 150px from bottom (scaled by DPI)
- **Size**: 80px radius outer circle, 30px radius inner knob
- **Function**: 
  - Drag in any direction to move
  - Forward = up, Back = down, Strafe = left/right
  - Returns to center when released
  - Dead zone prevents drift
- **Visual**: 
  - Semi-transparent gray circle (30% opacity)
  - White outline
  - White knob (70% opacity when active, 40% when idle)

### Fire Button (Bottom-Right)
- **Position**: Right edge margin, bottom margin
- **Size**: 70px × 70px (scaled by DPI)
- **Color**: Red
- **Label**: "FIRE"
- **Function**: Hold to shoot continuously
- **Opacity**: 60% idle, 90% pressed

### Jump Button (Above Fire Button)
- **Position**: Right edge margin, above fire button (1.2× spacing)
- **Size**: 70px × 70px (scaled by DPI)
- **Color**: Green
- **Label**: "JUMP"
- **Function**: Tap to jump
- **Opacity**: 60% idle, 90% pressed

### Pause Button (Top-Left)
- **Position**: 30px margin from top-left corner
- **Size**: 56px × 56px (0.8× standard button size)
- **Color**: Yellow
- **Label**: "||" (pause symbol)
- **Function**: Tap to pause game
- **Opacity**: 60% idle, 90% pressed

### Inventory Button (Top-Right)
- **Position**: Right edge margin, top margin
- **Size**: 70px × 70px (scaled by DPI)
- **Color**: Blue
- **Label**: "INV"
- **Function**: Tap to open/close inventory
- **Opacity**: 60% idle, 90% pressed

### Weapon Switch Button (Below Inventory)
- **Position**: Right edge margin, below inventory (1.2× spacing)
- **Size**: 70px × 70px (scaled by DPI)
- **Color**: Purple
- **Label**: "WPN"
- **Function**: Tap to cycle through owned weapons
- **Opacity**: 60% idle, 90% pressed

### Camera Control (Screen Drag)
- **Position**: Anywhere on screen except UI elements
- **Active Zone**: Primarily right 60% of screen
- **Function**: 
  - Touch and drag to rotate camera
  - Horizontal drag = yaw (look left/right)
  - Vertical drag = pitch (look up/down)
  - Continuous tracking while finger held
  - Sensitivity: 0.003 (adjustable)
- **Visual**: No visual indicator (transparent)
- **Priority**: Lower than buttons (buttons take precedence)

## Touch Handling Priority

1. **Highest**: UI Buttons (Pause, Inventory, Weapon)
2. **High**: Fire and Jump buttons
3. **Medium**: Movement joystick
4. **Low**: Camera drag (anywhere else)

This ensures buttons always respond even if you drag over them.

## Multi-Touch Support

The game supports up to 10 simultaneous touches:
- **Touch 0**: Usually movement joystick
- **Touch 1**: Usually camera drag
- **Touch 2+**: Additional buttons (fire, jump, etc.)

Each touch is tracked with a unique ID to prevent conflicts.

## DPI Scaling

All sizes scale with device DPI:
```
Scale Factor = (Device DPI) / 160
```

For example:
- **MDPI** (160 DPI): 1.0× scale (70px buttons)
- **HDPI** (240 DPI): 1.5× scale (105px buttons)
- **XHDPI** (320 DPI): 2.0× scale (140px buttons)
- **XXHDPI** (480 DPI): 3.0× scale (210px buttons)

This ensures consistent physical button sizes across different devices.

## Customization Ideas

Future enhancements could include:

1. **Settings Menu**:
   - Button size adjustment
   - Touch sensitivity slider
   - Button position presets (left-handed mode)
   - Opacity adjustment
   - Enable/disable individual controls

2. **Additional Controls**:
   - Sprint button (double-tap joystick alternative)
   - Quick-turn gesture (swipe left/right)
   - Zoom gesture (pinch)
   - Grenade throw arc indicator

3. **Visual Improvements**:
   - Button icons instead of text
   - Animated button press effects
   - Customizable button colors/themes
   - Haptic feedback on press

4. **Advanced Features**:
   - Gyroscope aiming
   - Controller support (Bluetooth gamepad)
   - Adaptive layout (portrait mode)
   - Tutorial overlay for first-time players

## Implementation Notes

- All touch controls are only visible during gameplay
- Hidden in menus, pause screen, shop, and inventory
- Controls automatically reposition on screen size changes
- Rounded corners (0.2 border radius) for modern look
- White outlines for visibility against any background
- Touch IDs tracked to prevent button conflicts
- Static booleans prevent button bouncing/double-triggers
