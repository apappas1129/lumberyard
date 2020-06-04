/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/UI/PropertyEditor/DHQSpinbox.hxx>

#include <QApplication>
#include <QLineEdit>

namespace UnitTest
{
    using namespace AzToolsFramework;

    // Expose the LineEdit functionality so selection behavior can be more easily tested.
    class DoubleSpinBoxWithLineEdit
        : public DHQDoubleSpinbox
    {
    public:
        // const required as lineEdit() is const
        QLineEdit* GetLineEdit() const { return lineEdit(); }
    };

    // A fixture to help test the int and double spin boxes.
    class SpinBoxFixture
        : public ToolsApplicationFixture
    {
    public:
        void SetUpEditorFixtureImpl() override
        {
            // note: must set a widget as the active window and add widgets
            // as children to ensure focus in/out events fire correctly
            m_dummyWidget = AZStd::make_unique<QWidget>();
            QApplication::setActiveWindow(m_dummyWidget.get());

            m_intSpinBox = AZStd::make_unique<DHQSpinbox>();
            m_doubleSpinBox = AZStd::make_unique<DHQDoubleSpinbox>();
            m_doubleSpinBoxWithLineEdit = AZStd::make_unique<DoubleSpinBoxWithLineEdit>();

            m_spinBoxes = { m_intSpinBox.get(), m_doubleSpinBox.get(), m_doubleSpinBoxWithLineEdit.get() };
            for (auto spinBox : m_spinBoxes)
            {
                spinBox->setParent(m_dummyWidget.get());
                spinBox->setKeyboardTracking(false);
                spinBox->setFocusPolicy(Qt::StrongFocus);
                spinBox->clearFocus();
            }
        }

        void TearDownEditorFixtureImpl() override
        {
            QApplication::setActiveWindow(nullptr);

            for (auto spinBox : m_spinBoxes)
            {
                spinBox->setParent(nullptr);
            }

            m_dummyWidget.reset();
            m_doubleSpinBoxWithLineEdit.reset();
            m_doubleSpinBox.reset();
            m_intSpinBox.reset();
        }

        AZStd::unique_ptr<QWidget> m_dummyWidget;
        AZStd::unique_ptr<DHQSpinbox> m_intSpinBox;
        AZStd::unique_ptr<DHQDoubleSpinbox> m_doubleSpinBox;
        AZStd::unique_ptr<DoubleSpinBoxWithLineEdit> m_doubleSpinBoxWithLineEdit;

        AZStd::vector<QAbstractSpinBox*> m_spinBoxes;
    };

    TEST_F(SpinBoxFixture, SpinBoxesCreated)
    {
        using ::testing::Ne;

        EXPECT_THAT(m_intSpinBox, Ne(nullptr));
        EXPECT_THAT(m_doubleSpinBox, Ne(nullptr));
        EXPECT_THAT(m_doubleSpinBoxWithLineEdit, Ne(nullptr));
    }

    // Note: There are a series of bugs in Qt that appear to be preventing mouseMove events
    // firing when sent through the QTest framework. This is a work around for our version
    // of Qt. In future this can hopefully be simplified. See ^1 for workaround.
    // More info: Issues with mouse move in Qt
    // - https://bugreports.qt.io/browse/QTBUG-5232
    // - https://bugreports.qt.io/browse/QTBUG-69414
    // - https://lists.qt-project.org/pipermail/development/2019-July/036873.html
    void MousePressAndMove(
        QWidget* widget, const QPoint& widgetScreenPosition, const QPoint& mouseDelta)
    {
        QPoint position = widget->mapToGlobal(widgetScreenPosition);
        QPoint nextPosition = widget->mapToGlobal(widgetScreenPosition + mouseDelta);

        QTest::mousePress(widget, Qt::LeftButton, Qt::NoModifier, position);

        // ^1 To ensure a mouse move event is fired we must call the test mouse move function
        // and also send a mouse move event that matches. Each on their own do not appear to
        // work - please see the links above for more context.
        QTest::mouseMove(widget, nextPosition);
        QMouseEvent mouseMoveEvent(
            QEvent::MouseMove, QPointF(nextPosition), QPointF(nextPosition),
            Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(widget, &mouseMoveEvent);
    }

    TEST_F(SpinBoxFixture, SpinBoxMousePressAndMoveDownScrollsValue)
    {
        m_doubleSpinBox->setValue(10.0);

        const int halfWidgetHeight = m_doubleSpinBox->height() / 2;
        const QPoint widgetCenterLeftBorder = m_doubleSpinBox->pos() + QPoint(1, halfWidgetHeight);

        // down in screen space
        MousePressAndMove(m_doubleSpinBox.get(), widgetCenterLeftBorder, QPoint(0, 10));

        EXPECT_NEAR(m_doubleSpinBox->value(), 0.0, 0.001);
    }

    TEST_F(SpinBoxFixture, SpinBoxMousePressAndMoveUpScrollsValue)
    {
        m_doubleSpinBox->setValue(10.0);

        const int halfWidgetHeight = m_doubleSpinBox->height() / 2;
        const QPoint widgetCenterLeftBorder = m_doubleSpinBox->pos() + QPoint(1, halfWidgetHeight);

        // up in screen space
        MousePressAndMove(m_doubleSpinBox.get(), widgetCenterLeftBorder, QPoint(0, -10));

        EXPECT_NEAR(m_doubleSpinBox->value(), 20.0, 0.001);
    }

    TEST_F(SpinBoxFixture, SpinBoxKeyboardUpAndDownArrowsChangeValue)
    {
        m_intSpinBox->setValue(5);
        m_intSpinBox->setFocus();

        QTest::keyClick(m_intSpinBox.get(), Qt::Key_Up, Qt::NoModifier);

        EXPECT_EQ(m_intSpinBox->value(), 6);

        QTest::keyClick(m_intSpinBox.get(), Qt::Key_Down, Qt::NoModifier);
        QTest::keyClick(m_intSpinBox.get(), Qt::Key_Down, Qt::NoModifier);

        EXPECT_EQ(m_intSpinBox->value(), 4);
    }

    TEST_F(SpinBoxFixture, SpinBoxChangeContentsAndEnterCommitsNewValue)
    {
        m_doubleSpinBoxWithLineEdit->setValue(10.0);
        m_doubleSpinBoxWithLineEdit->setFocus();
        m_doubleSpinBoxWithLineEdit->GetLineEdit()->setText(QString("15"));

        QTest::keyClick(m_doubleSpinBoxWithLineEdit.get(), Qt::Key_Enter, Qt::NoModifier);

        EXPECT_NEAR(m_doubleSpinBoxWithLineEdit->value(), 15.0, 0.001);
    }

    TEST_F(SpinBoxFixture, SpinBoxChangeContentsAndLoseFocusCommitsNewValue)
    {
        m_doubleSpinBoxWithLineEdit->setValue(10.0);
        m_doubleSpinBoxWithLineEdit->setFocus();
        m_doubleSpinBoxWithLineEdit->GetLineEdit()->setText(QString("15"));

        m_doubleSpinBoxWithLineEdit->clearFocus();

        EXPECT_NEAR(m_doubleSpinBoxWithLineEdit->value(), 15.0, 0.001);
    }

    TEST_F(SpinBoxFixture, SpinBoxClearContentsAndEscapeReturnsToPreviousValue)
    {
        m_doubleSpinBoxWithLineEdit->setValue(10.0);
        m_doubleSpinBoxWithLineEdit->setFocus();
        m_doubleSpinBoxWithLineEdit->GetLineEdit()->clear();

        QTest::keyClick(m_doubleSpinBoxWithLineEdit.get(), Qt::Key_Escape, Qt::NoModifier);

        EXPECT_NEAR(m_doubleSpinBoxWithLineEdit->value(), 10.0, 0.001);
    }

    TEST_F(SpinBoxFixture, SpinBoxChangeContentsAndEscapeReturnsToPreviousValue)
    {
        m_doubleSpinBoxWithLineEdit->setValue(10.0);
        m_doubleSpinBoxWithLineEdit->setFocus();
        m_doubleSpinBoxWithLineEdit->GetLineEdit()->setText(QString("15"));

        QTest::keyClick(m_doubleSpinBoxWithLineEdit.get(), Qt::Key_Escape, Qt::NoModifier);

        EXPECT_NEAR(m_doubleSpinBoxWithLineEdit->value(), 10.0, 0.001);
        EXPECT_TRUE(m_doubleSpinBoxWithLineEdit->GetLineEdit()->hasSelectedText());
    }

    TEST_F(SpinBoxFixture, SpinBoxSelectContentsAndEscapeClearsFocus)
    {
        m_doubleSpinBox->setValue(10.0);
        m_doubleSpinBox->setFocus();
        m_doubleSpinBox->selectAll();

        QTest::keyClick(m_doubleSpinBox.get(), Qt::Key_Escape, Qt::NoModifier);

        EXPECT_FALSE(m_doubleSpinBox->hasFocus());
    }

    TEST_F(SpinBoxFixture, SpinBoxSuffixRemovedAndAppliedWithFocusChange)
    {
        using testing::StrEq;

        m_doubleSpinBox->setSuffix("m");
        m_doubleSpinBox->setValue(10.0);

        // test internal logic (textFromValue() calls private StringValue())
        QString value = m_doubleSpinBox->textFromValue(10.0);
        EXPECT_THAT(value.toUtf8().constData(), StrEq("10.0"));

        m_doubleSpinBox->setFocus();
        EXPECT_THAT(m_doubleSpinBox->suffix().toUtf8().constData(), StrEq(""));

        m_doubleSpinBox->clearFocus();
        EXPECT_THAT(m_doubleSpinBox->suffix().toUtf8().constData(), StrEq("m"));
    }
} // namespace UnitTest