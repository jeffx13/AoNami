pragma Singleton
import QtQuick

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
        "grass": {
            light: true,
            accent: "#4E9F3D",
            background: "#F3F7F1", bgBottom: "#E7EFE4",
            surface: "#FFFFFF", surfaceAlt: "#EDF3EA", surfaceDeep: "#E6EEE2", border: "#D3DFCD",
            textPrimary: "#1B2A1B", textSecondary: "#41513F", textMuted: "#6D7C6A",
            success: "#2E9E6B", danger: "#D64545"
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

    // Light themes flip the accent derivations darker so they stay visible on a light surface.
    readonly property bool isLight: pal.light === true

    // Accent is decoupled from the palette so a custom accent works on every theme.
    readonly property string accent:        customAccent !== "" ? customAccent : pal.accent
    readonly property string accentLight:   isLight ? Qt.darker(accent, 1.15) : Qt.lighter(accent, 1.25)
    readonly property string textAccent:    isLight ? Qt.darker(accent, 1.5)  : Qt.lighter(accent, 1.6)

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
