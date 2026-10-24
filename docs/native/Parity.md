# LVRS 네이티브 이식 대응표

기준은 현재 QML·C++ 구현 전체이다. `partial`은 지원 입력과 기본 동작만 실제로 구현했다는 뜻이며 원본과 완전 동등함을 의미하지 않는다. 모든 항목이 요구 범위에 남아 있다.

- 렌더링: iiGUI 자체 C++ 위젯과 GPU 그리기 명령
- OS 경계: 창·이벤트·IME·접근성만 연결
- 앱 수명과 최종 실행 파일: iiSDKCore

| 선언 | 종류 | 상태 | 네이티브 API | 직접 노출 멤버 수 |
|---|---|---|---|---:|
| FontPolicy | nonvisual | pending | — | 11 |
| ColorPickerModel | nonvisual | partial | lvrs::ColorPickerModel / iiGUI native model | 15 |
| ColorPickerSurface | view | partial | lvrs::Widgets::color_surface | 2 |
| SvgManager | nonvisual | pending | — | 10 |
| WindowNoiseTexture | view | pending | — | 5 |
| Backend | nonvisual | pending | — | 41 |
| HierarchyModel | nonvisual | partial | lvrs::HierarchyModel / HierarchyProjection | 30 |
| ModelSource | nonvisual | pending | — | 12 |
| ModelUndoStack | nonvisual | pending | — | 9 |
| ProgressModel | nonvisual | pending | — | 24 |
| TableHeaderModel | nonvisual | partial | lvrs::TableHeaderModel | 23 |
| TableModel | nonvisual | partial | lvrs::TableModel | 106 |
| ModelAdapter | nonvisual | pending | — | 3 |
| NavigationStackModel | nonvisual | partial | lvrs::NavigationStackModel | 21 |
| PageMonitor | nonvisual | partial | lvrs::PageMonitor | 7 |
| RouteMatcher | nonvisual | partial | lvrs::RouteMatcher | 2 |
| RouteResolver | nonvisual | partial | lvrs::RouteResolver | 3 |
| ViewStateTracker | nonvisual | partial | lvrs::ViewStateTracker / Navigator tracking domain | 15 |
| NativeWindowInteraction | nonvisual | pending | — | 4 |
| NativeWindowStyle | nonvisual | pending | — | 8 |
| Platform | nonvisual | pending | — | 27 |
| WindowSafeAreaObserver | nonvisual | pending | — | 8 |
| Debug | nonvisual | pending | — | 29 |
| GestureEvents | nonvisual | pending | — | 12 |
| RenderMonitor | nonvisual | pending | — | 16 |
| RenderQuality | nonvisual | pending | — | 49 |
| RuntimeEvents | nonvisual | pending | — | 66 |
| AppState | nonvisual | pending | — | 23 |
| StateModel | nonvisual | pending | — | 12 |
| ViewModel | nonvisual | pending | — | 8 |
| ViewModels | nonvisual | partial | LVUI scoped C++ backends / Core Observable | 24 |
| TextDocumentModel | nonvisual | partial | lvrs::TextDocumentModel / TextDocumentStorage | 45 |
| TextMarkup | nonvisual | partial | lvrs::TextMarkup | 3 |
| AppBootstrapWindow | view | pending | — | 0 |
| AppShell | view | pending | — | 0 |
| ApplicationWindow | view | partial | Core Application + iiGUI Host | 174 |
| AdaptiveLayoutHost | view (internal) | pending | — | 85 |
| Motion | nonvisual | partial | lvrs::Motion / MotionTokens / explicit AOT Alert/Modal/Sheet model | 13 |
| Theme | nonvisual | partial | lvrs::Theme + lvrs::Icons | 293 |
| Window | view | partial | Core Application + iiGUI Host | 58 |
| WindowChromeInteraction | view | pending | — | 25 |
| AbstractButton | view | partial | lvrs::Widgets::button | 36 |
| ButtonMethodRegistry | nonvisual | pending | — | 10 |
| ColorPickerButton | view | partial | lvrs::Widgets::color_picker_button / iiGUI native button | 4 |
| ComboBox | view | partial | lvrs::Widgets::combo_box | 38 |
| DropdownButton | view | partial | lvrs::Widgets::dropdown_button | 22 |
| HelpButton | view | partial | lvrs::Widgets::help_button | 0 |
| IconButton | view | partial | lvrs::Widgets::icon_button | 0 |
| IconMenuButton | view | partial | lvrs::Widgets::icon_menu_button | 0 |
| IconSegmentedControl | view | partial | lvrs::Widgets::icon_segmented_control | 26 |
| LabelButton | view | partial | lvrs::Widgets::button | 0 |
| LabelMenuButton | view | partial | lvrs::Widgets::dropdown_button | 0 |
| LabelSegmentedControl | view | partial | lvrs::Widgets::label_segmented_control | 26 |
| PushButton | view | partial | lvrs::Widgets::button | 14 |
| Stepper | view | partial | lvrs::Widgets::stepper | 53 |
| CheckBox | view | partial | lvrs::Widgets::check_box | 37 |
| RadioButton | view | partial | lvrs::Widgets::radio_button | 14 |
| ToggleSwitch | view | partial | lvrs::Widgets::toggle_switch | 33 |
| Avatar | view | partial | lvrs::Avatar / Widgets::avatar / LVUI AOT | 8 |
| Label | view | partial | lvrs::Widgets::label | 36 |
| ProgressBar | view | partial | lvrs::Widgets::progress | 40 |
| Table | view | partial | lvrs::Table / Widgets::table / AOT | 227 |
| TableCellItem | view | partial | lvrs::TableCellItem / Widgets::table_cell_item / AOT | 45 |
| TableHeader | view | partial | lvrs::TableHeader / Widgets::table_header / AOT | 32 |
| TableRow | view | partial | lvrs::TableRow / Widgets::table_row / AOT | 16 |
| AbstractInputBar | view | pending | — | 93 |
| CodeEditor | view | partial | lvrs::CodeEditor / Widgets::code_editor / LVUI AOT | 99 |
| ColorPicker | view | partial | lvrs::ColorPicker / Widgets::color_picker | 23 |
| ColorInput | view (internal) | pending | — | 4 |
| Swatch | view (internal) | pending | — | 3 |
| ChannelRow | view (internal) | pending | — | 3 |
| ColorField | view (internal) | partial | lvrs::Widgets::color_surface | 0 |
| InputField | view | partial | lvrs::Widgets::input_field | 40 |
| MaterialSurface | view (internal) | pending | — | 13 |
| Slider | view | partial | lvrs::Widgets::slider | 50 |
| Endpoint | view (internal) | pending | — | 6 |
| Material | view (internal) | pending | — | 4 |
| TextEditor | view | pending | — | 103 |
| EventListener | view | partial | lvrs::Widgets::event_listener / AOT | 46 |
| FocusRing | view | pending | — | 2 |
| InputMethodGuard | view | pending | — | 8 |
| InteractionMotion | modifier | pending | — | 19 |
| InteractionState | nonvisual | pending | — | 9 |
| SpringBehavior | modifier | pending | — | 4 |
| StateColorBehavior | modifier | pending | — | 1 |
| WheelScrollGuard | view | pending | — | 7 |
| AppHeader | view | partial | lvrs::AppHeader / Widgets::app_header / AOT | 11 |
| HStack | view | partial | lvrs::Widgets::hstack | 12 |
| Spacer | view | partial | lvrs::Widgets::spacer | 9 |
| VStack | view | partial | lvrs::Widgets::vstack | 12 |
| ZStack | view | partial | lvrs::Widgets::zstack | 10 |
| ContextMenu | view | partial | lvrs::Widgets::context_menu | 96 |
| ContextMenuDivider | view | partial | lvrs::Widgets::context_menu_divider | 0 |
| ContextMenuItem | view | partial | lvrs::Widgets::context_menu_item | 0 |
| Hierarchy | view | partial | lvrs::List / ListModel / Widgets::hierarchy / AOT | 56 |
| HierarchyItem | view | pending | — | 205 |
| HierarchyList | view | pending | — | 201 |
| HierarchyToolbar | view | partial | lvrs::HierarchyToolbar / Widgets::hierarchy_toolbar / AOT | 62 |
| Link | view | partial | lvrs::Link / Widgets::link / LVUI AOT | 14 |
| List | view | partial | lvrs::List / ListModel / Widgets::list / AOT | 57 |
| ListFooter | view | partial | lvrs::ListFooter / Widgets::list_footer / AOT | 22 |
| ListItem | view | partial | lvrs::Widgets::list_item / AOT | 137 |
| ListItemComposite | view | pending | — | 3 |
| BodyText | view (internal) | pending | — | 0 |
| CaptionText | view (internal) | pending | — | 1 |
| Glyph | view (internal) | pending | — | 3 |
| Slot | view (internal) | pending | — | 3 |
| ContentText | view (internal) | pending | — | 1 |
| ActionSlot | view (internal) | pending | — | 1 |
| MoreSlot | view (internal) | pending | — | 0 |
| ToggleSlot | view (internal) | pending | — | 0 |
| SelectionSlot | view (internal) | pending | — | 0 |
| SelectorSlot | view (internal) | pending | — | 1 |
| QuantityControl | view (internal) | pending | — | 0 |
| InputSlot | view (internal) | pending | — | 1 |
| QuantitySlot | view (internal) | pending | — | 0 |
| SegmentsSlot | view (internal) | pending | — | 0 |
| PreviewSlot | view (internal) | pending | — | 0 |
| ListItemSelector | view | pending | — | 10 |
| ListToolbar | view | partial | lvrs::ListToolbar / Widgets::list_toolbar / AOT | 20 |
| Menu | view | partial | lvrs::Widgets::menu | 0 |
| MenuDivider | view | partial | lvrs::Widgets::menu_divider | 8 |
| MenuItem | view | partial | lvrs::Widgets::menu_item | 48 |
| MobileNavigationBar | view | pending | — | 29 |
| MobileNavigationTab | view | pending | — | 5 |
| MobileTab | view | pending | — | 7 |
| MobileTabBar | view | pending | — | 10 |
| Navigator | nonvisual | partial | lvrs::Navigator / NavigationTarget | 32 |
| PageRouter | view | partial | lvrs::PageRouter / Widgets::page_router / LVUI AOT | 105 |
| PageRouterTransitionDriver | nonvisual | pending | — | 67 |
| PageTransitionController | nonvisual | pending | — | 29 |
| Tab | view | partial | lvrs::Widgets::tab | 16 |
| TabBar | view | partial | lvrs::Widgets::tab_bar | 21 |
| ToolbarButton | view | partial | lvrs::ToolbarButton / Widgets::toolbar_button / AOT | 4 |
| Alert | view | partial | lvrs::Alert / Widgets::alert / AOT methods | 62 |
| AlertButton | view | partial | lvrs::AlertButton / Widgets::alert_button / AOT methods | 3 |
| AppCard | view | partial | lvrs::AppCard / Widgets::app_card / AOT | 13 |
| Card | view | partial | lvrs::Card / Widgets::card / LVUI AOT | 50 |
| CardImageContent | view (internal) | pending | — | 8 |
| CardInformationContent | view (internal) | pending | — | 1 |
| DetailRow | view (internal) | pending | — | 3 |
| MaterialSurface | view | partial | lvrs::MaterialSurface / Widgets::material_surface / LVUI AOT | 35 |
| RadialLayer | view (internal) | partial | lvrs::material_detail native gradient raster | 5 |
| Modal | view | partial | lvrs::Modal / Widgets::modal / AOT activation content | 59 |
| PanelMaterial | view | partial | lvrs::PanelMaterial / Widgets::panel_material / LVUI AOT | 0 |
| Popover | view | partial | lvrs::Widgets::popover / iiGUI native overlay | 8 |
| Sheet | view | partial | lvrs::Sheet / Widgets::sheet / AOT retained content | 43 |
| Tooltip | view | partial | lvrs::Tooltip / TooltipLayout / Widgets::tooltip / LVUI AOT | 58 |
| WindowMaterial | view | partial | lvrs::WindowMaterial / Widgets::window_material / LVUI AOT | 0 |

각 항목의 원본 위치·직접 노출 멤버·지원 입력·남은 기능은 `catalog/native/parity.json`에 기록한다. 상속 API, 동작, 렌더링 품질과 플랫폼 검증을 모두 충족하기 전에는 `complete`로 바꾸지 않는다.
