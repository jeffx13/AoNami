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
            textPrimary: "#1B2A1B", textSecondary: "#3A4838", textMuted: "#414B3C",
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
        "slate": {
            accent: "#60A5FA",
            background: "#0F172A", bgBottom: "#070B16",
            surface: "#1E293B", surfaceAlt: "#293548", surfaceDeep: "#0B1120", border: "#334155",
            textPrimary: "#E2E8F0", textSecondary: "#B6C2D4", textMuted: "#7488A0",
            success: "#34D399", danger: "#FB7185"
        },
        "glacier": {
            accent: "#22D3EE",
            background: "#0A1622", bgBottom: "#050C14",
            surface: "#10212E", surfaceAlt: "#163240", surfaceDeep: "#07131C", border: "#1E3A4C",
            textPrimary: "#E0F2FE", textSecondary: "#A9CCDD", textMuted: "#6B8A9C",
            success: "#34D399", danger: "#FB7185"
        },
        "frost": {
            light: true,
            accent: "#2563EB",
            background: "#F2F6FC", bgBottom: "#E4ECF7",
            surface: "#FFFFFF", surfaceAlt: "#EAF1FB", surfaceDeep: "#E1EAF6", border: "#CED9EA",
            textPrimary: "#15233B", textSecondary: "#35455E", textMuted: "#3E4C66",
            success: "#2E9E6B", danger: "#D64545"
        },
        "mist": {
            light: true,
            accent: "#0EA5E9",
            background: "#F4F6F8", bgBottom: "#E8ECF0",
            surface: "#FFFFFF", surfaceAlt: "#EDF1F4", surfaceDeep: "#E5EAEF", border: "#D5DCE3",
            textPrimary: "#1A2330", textSecondary: "#3A4552", textMuted: "#424E5C",
            success: "#2E9E6B", danger: "#D64545"
        },
        "seafoam": {
            light: true,
            accent: "#0D9488",
            background: "#F0F8F6", bgBottom: "#E2F0EC",
            surface: "#FFFFFF", surfaceAlt: "#E7F4F0", surfaceDeep: "#DEEEEA", border: "#CCE2DC",
            textPrimary: "#143029", textSecondary: "#2E4842", textMuted: "#3A4D45",
            success: "#2E9E6B", danger: "#D64545"
        },
        "lavender": {
            light: true,
            accent: "#7C3AED",
            background: "#F6F4FC", bgBottom: "#ECE7F8",
            surface: "#FFFFFF", surfaceAlt: "#F0EBFA", surfaceDeep: "#E8E1F5", border: "#DDD3EE",
            textPrimary: "#241B38", textSecondary: "#3F3556", textMuted: "#483F5C",
            success: "#2E9E6B", danger: "#D64545"
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
