// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QDESIGNER_WORKBENCH_H
#define QDESIGNER_WORKBENCH_H

#include "designer_enums.h"

#include <QtCore/qobject.h>
#include <QtCore/qhash.h>
#include <QtCore/qlist.h>
#include <QtCore/qset.h>
#include <QtCore/qrect.h>

QT_BEGIN_NAMESPACE

class QDesignerActions;
class QDesignerToolWindow;
class QDesignerFormWindow;
class DockedMainWindow;
class QDesignerSettings;

class QAction;
class QActionGroup;
class QDockWidget;
class QMenu;
class QMenuBar;
class QMainWindow;
class QToolBar;
class QMdiArea;
class QMdiSubWindow;
class QCloseEvent;
class QFont;
class QtToolBarManager;
class ToolBarManager;

class QDesignerFormEditorInterface;
class QDesignerFormWindowInterface;
class QDesignerFormWindowManagerInterface;
class QDesignerIntegration;

class QDesignerWorkbench: public QObject
{
    Q_OBJECT

public:
    QDesignerWorkbench();
    ~QDesignerWorkbench() override;

    UIMode mode() const;

    QDesignerFormEditorInterface *core() const;
    QDesignerFormWindow *findFormWindow(QWidget *widget) const;

    QDesignerFormWindow *openForm(const QString &fileName, QString *errorMessage);
    QDesignerFormWindow *openTemplate(const QString &templateFileName,
                                      const QString &editorFileName,
                                      QString *errorMessage);

    int toolWindowCount() const;
    QDesignerToolWindow *toolWindow(int index) const;

    int formWindowCount() const;
    QDesignerFormWindow *formWindow(int index) const;

    QDesignerActions *actionManager() const;

    QActionGroup *modeActionGroup() const;

    QRect availableGeometry() const;
    QRect desktopGeometry() const;

    int marginHint() const;

    bool readInForm(const QString &fileName) const;
    bool writeOutForm(QDesignerFormWindowInterface *formWindow, const QString &fileName) const;
    bool saveForm(QDesignerFormWindowInterface *fw);
    bool handleClose();
    bool readInBackup();
    void updateBackup(QDesignerFormWindowInterface* fwi);
    void applyUiSettings();

signals:
    void modeChanged(UIMode mode);
    void initialized();

public slots:
    void addFormWindow(QDesignerFormWindow *formWindow);
    void removeFormWindow(QDesignerFormWindow *formWindow);
    void bringAllToFront();
    void toggleFormMinimizationState();

private slots:
    void switchToNeutralMode();
    void switchToDockedMode();
    void switchToTopLevelMode();
    void initializeCorePlugins();
    void handleCloseEvent(QCloseEvent *);
    void slotFormWindowActivated(QDesignerFormWindow* fw);
    void updateWindowMenu(QDesignerFormWindowInterface *fw);
    void formWindowActionTriggered(QAction *a);
    void adjustMDIFormPositions();
    void minimizationStateChanged(QDesignerFormWindowInterface *formWindow, bool minimized);

    void restoreUISettings();
    void notifyUISettingsChanged();
    void slotFileDropped(const QString &f);

private:
    QWidget *magicalParent(const QWidget *w) const;
    Qt::WindowFlags magicalWindowFlags(const QWidget *widgetForFlags) const;
    QDesignerFormWindowManagerInterface *formWindowManager() const;
    void closeAllToolWindows();
    QDesignerToolWindow *widgetBoxToolWindow() const;
    QDesignerFormWindow *loadForm(const QString &fileName, bool detectLineTermiantorMode, QString *errorMessage);
    void resizeForm(QDesignerFormWindow *fw,  const QWidget *mainContainer) const;
    void saveGeometriesForModeChange();
    void saveGeometries(QDesignerSettings &settings) const;

    bool isFormWindowMinimized(const QDesignerFormWindow *fw);
    void setFormWindowMinimized(QDesignerFormWindow *fw, bool minimized);
    void saveSettings() const;

    QDesignerFormEditorInterface *m_core;
    QDesignerIntegration *m_integration;

    QDesignerActions *m_actionManager;
    QActionGroup *m_windowActions;

    QMenu *m_windowMenu;

    QMenuBar *m_globalMenuBar;

    struct TopLevelData {
        ToolBarManager *toolbarManager;
        QList<QToolBar *> toolbars;
    };
    TopLevelData m_topLevelData;

    UIMode m_mode = NeutralMode;
    DockedMainWindow *m_dockedMainWindow = nullptr;

    QList<QDesignerToolWindow *> m_toolWindows;
    QList<QDesignerFormWindow *> m_formWindows;

    QMenu *m_toolbarMenu;

    // Helper class to remember the position of a window while switching user
    // interface modes.
    class Position {
    public:
        Position(const QDockWidget *dockWidget);
        Position(const QMdiSubWindow *mdiSubWindow, const QPoint &mdiAreaOffset);
        Position(const QWidget *topLevelWindow, const QPoint &desktopTopLeft);

        void applyTo(QMdiSubWindow *mdiSubWindow, const QPoint &mdiAreaOffset) const;
        void applyTo(QWidget *topLevelWindow, const QPoint &desktopTopLeft) const;
        void applyTo(QDockWidget *dockWidget) const;

        QPoint position() const { return m_position; }
    private:
        bool m_minimized;
        // Position referring to top-left corner (desktop in top-level mode or
        // main window in MDI mode)
        QPoint m_position;
    };
    using PositionMap = QHash<QWidget*, Position>;
    PositionMap m_Positions;

    enum State { StateInitializing, StateUp, StateClosing };
    State m_state = StateInitializing;
    bool m_uiSettingsChanged = false; // UI mode changed in preference dialog, trigger delayed slot.
};

QT_END_NAMESPACE

#endif // QDESIGNER_WORKBENCH_H
