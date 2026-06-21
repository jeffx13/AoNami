pragma Singleton
import QtQml

// Central color palette - all UI colors reference Theme.* so the app re-skins from here.
QtObject {
    id: theme

    property string mode: "dark"

    // Brand
    readonly property string accent:        "#4E5BF2"
    readonly property string accentLight:   "#6B78F5"

    // Surfaces
    readonly property string background:    "#0B1220"
    readonly property string surface:       "#0F172A"
    readonly property string surfaceAlt:    "#111827"
    readonly property string surfaceDeep:   "#0A0F1E"
    readonly property string border:        "#1E293B"

    // Text
    readonly property string textPrimary:   "#E5E7EB"
    readonly property string textSecondary: "#CBD5E1"
    readonly property string textMuted:     "#6B7280"
    readonly property string textAccent:    "#BFC7FF"

    // Status
    readonly property string success:       "#10B981"
    readonly property string danger:        "#EF4444"
}
