pragma Singleton
import QtQuick

// Central color palette - all UI colors reference Theme.* so the app re-skins from here.
// `name` and `customAccent` are driven from settings by main.qml. `background` -> `bgBottom`
// is the subtle backdrop gradient drawn behind the pages.
QtObject {
    id: theme

    property string name: "obsidian"
    property string customAccent: ""   // empty = use the theme's own accent

    readonly property var palettes: ({
        "obsidian": {
            accent: "#7C83FF",
            background: "#10142E", bgBottom: "#07091A",
            surface: "#171D3D", surfaceAlt: "#1F2750", surfaceDeep: "#0B0F26", border: "#2C3563",
            textPrimary: "#ECEEFF", textSecondary: "#C3C8EC", textMuted: "#767BA6",
            success: "#34D399", danger: "#FB7185"
        },
        "emerald": {
            accent: "#34D8A0",
            background: "#0C1F18", bgBottom: "#050D0A",
            surface: "#11291F", surfaceAlt: "#18382B", surfaceDeep: "#07140F", border: "#214A39",
            textPrimary: "#E8F3ED", textSecondary: "#BCD6C9", textMuted: "#6F8C80",
            success: "#6EE7B7", danger: "#F87171"
        },
        "amethyst": {
            accent: "#A78BFA",
            background: "#1A1338", bgBottom: "#0C0820",
            surface: "#221944", surfaceAlt: "#2D2258", surfaceDeep: "#110C28", border: "#382C68",
            textPrimary: "#F1ECFF", textSecondary: "#CFC6EC", textMuted: "#8479A8",
            success: "#34D399", danger: "#FB7185"
        },
        "onyx": {
            accent: "#6D8BFF",
            background: "#08080B", bgBottom: "#000000",
            surface: "#0D0D11", surfaceAlt: "#17171C", surfaceDeep: "#000000", border: "#24242B",
            textPrimary: "#F2F2F5", textSecondary: "#BEBEC6", textMuted: "#76767F",
            success: "#10B981", danger: "#EF4444"
        },
        "champagne": {
            accent: "#D9B06A",
            background: "#1E1813", bgBottom: "#120D09",
            surface: "#2A211A", surfaceAlt: "#352A20", surfaceDeep: "#140F0A", border: "#3F342A",
            textPrimary: "#F7F0E6", textSecondary: "#DBC9B4", textMuted: "#9B8A74",
            success: "#A3BE8C", danger: "#E08A6B"
        }
    })

    readonly property var pal: palettes[name] ? palettes[name] : palettes["obsidian"]

    // Accent is decoupled from the palette so a custom accent works on every theme.
    readonly property string accent:        customAccent !== "" ? customAccent : pal.accent
    readonly property string accentLight:   Qt.lighter(accent, 1.25)
    readonly property string textAccent:    Qt.lighter(accent, 1.6)

    readonly property string background:    pal.background
    readonly property string bgBottom:      pal.bgBottom
    readonly property string surface:       pal.surface
    readonly property string surfaceAlt:    pal.surfaceAlt
    readonly property string surfaceDeep:   pal.surfaceDeep
    readonly property string border:        pal.border

    readonly property string textPrimary:   pal.textPrimary
    readonly property string textSecondary: pal.textSecondary
    readonly property string textMuted:     pal.textMuted

    readonly property string success:       pal.success
    readonly property string danger:        pal.danger
}
