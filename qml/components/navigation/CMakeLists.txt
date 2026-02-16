set(LVRS_NAVIGATION_QML_FILES
    qml/components/navigation/Navigator.qml
    qml/components/navigation/PageRouter.qml
    qml/components/navigation/Link.qml
    qml/components/navigation/ToolbarButton.qml
    qml/components/navigation/HierarchyToolbar.qml
    qml/components/navigation/HierarchyItem.qml
    qml/components/navigation/HierarchyList.qml
    qml/components/navigation/Hierarchy.qml
    qml/components/navigation/List.qml
    qml/components/navigation/ListItem.qml
    qml/components/navigation/ListFooter.qml
    qml/components/navigation/ListToolbar.qml
    qml/components/navigation/ContextMenu.qml
    qml/components/navigation/MenuItem.qml
    qml/components/navigation/MenuDivider.qml
)

set_source_files_properties(
    qml/components/navigation/Navigator.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)
