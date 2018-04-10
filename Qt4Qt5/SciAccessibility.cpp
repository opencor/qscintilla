// The implementation of the class that implements accessibility support.
//
// Copyright (c) 2018 Riverbank Computing Limited <info@riverbankcomputing.com>
// 
// This file is part of QScintilla.
// 
// This file may be used under the terms of the GNU General Public License
// version 3.0 as published by the Free Software Foundation and appearing in
// the file LICENSE included in the packaging of this file.  Please review the
// following information to ensure the GNU General Public License version 3.0
// requirements will be met: http://www.gnu.org/copyleft/gpl.html.
// 
// If you do not wish to use this file under the terms of the GPL version 3.0
// then you may purchase a commercial license.  For more information contact
// info@riverbankcomputing.com.
// 
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.


#include <qglobal.h>

#if !defined(QT_NO_ACCESSIBILITY)

#include "SciAccessibility.h"

#include <QAccessible>
#include <QFont>
#include <QFontMetrics>
#include <QRect>
#include <QWidget>

#include "Qsci/qsciscintillabase.h"


// Set if the accessibility support needs initialising.
bool QsciAccessibleScintillaBase::needs_initialising = true;

// The list of all accessibles.
QList<QsciAccessibleScintillaBase *> QsciAccessibleScintillaBase::all_accessibles;


// Forward declarations.
static QAccessibleInterface *factory(const QString &classname, QObject *object);


// The ctor.
QsciAccessibleScintillaBase::QsciAccessibleScintillaBase(QWidget *widget) :
        QAccessibleWidget(widget, QAccessible::EditableText),
        current_cursor_position(-1), is_selection(false)
{
    all_accessibles.append(this);
}


// The dtor.
QsciAccessibleScintillaBase::~QsciAccessibleScintillaBase()
{
    all_accessibles.removeOne(this);
}


// Initialise the accessibility support.
void QsciAccessibleScintillaBase::initialise()
{
    if (needs_initialising)
    {
        QAccessible::installFactory(factory);
        needs_initialising = false;
    }
}


// Find the accessible for a widget.
QsciAccessibleScintillaBase *QsciAccessibleScintillaBase::findAccessible(
        QsciScintillaBase *sb)
{
    for (int i = 0; i < all_accessibles.size(); ++i)
    {
        QsciAccessibleScintillaBase *acc_sb = all_accessibles.at(i);

        if (acc_sb->sciWidget() == sb)
            return acc_sb;
    }

    return 0;
}


// Return the QsciScintillaBase instance.
QsciScintillaBase *QsciAccessibleScintillaBase::sciWidget() const
{
    return static_cast<QsciScintillaBase *>(widget());
}


// Update the accessible when the selection has changed.
void QsciAccessibleScintillaBase::selectionChanged(QsciScintillaBase *sb,
        bool selection)
{
    QsciAccessibleScintillaBase *acc_sb = findAccessible(sb);

    if (!acc_sb)
        return;

    acc_sb->is_selection = selection;
}


// Update the accessibility when text has been inserted.
void QsciAccessibleScintillaBase::textInserted(QsciScintillaBase *sb,
        int position, const char *text, int length)
{
    Q_ASSERT(text);

    QString new_text = bytesAsText(sb, text, length);
    int text_position = positionAsTextPosition(sb, position);

    QAccessibleTextInsertEvent ev(sb, text_position, new_text);
    QAccessible::updateAccessibility(&ev);
}


// Update the accessibility when text has been deleted.
void QsciAccessibleScintillaBase::textDeleted(QsciScintillaBase *sb,
        int position, const char *text, int length)
{
    Q_ASSERT(text);

    QString old_text = bytesAsText(sb, text, length);
    int text_position = positionAsTextPosition(sb, position);

    QAccessibleTextRemoveEvent ev(sb, text_position, old_text);
    QAccessible::updateAccessibility(&ev);
}


// Update the accessibility when the UI has been updated.
void QsciAccessibleScintillaBase::updated(QsciScintillaBase *sb)
{
    QsciAccessibleScintillaBase *acc_sb = findAccessible(sb);

    if (!acc_sb)
        return;

    int cursor_position = positionAsTextPosition(sb,
            sb->SendScintilla(QsciScintillaBase::SCI_GETCURRENTPOS));

    if (acc_sb->current_cursor_position != cursor_position)
    {
        acc_sb->current_cursor_position = cursor_position;

        QAccessibleTextCursorEvent ev(sb, cursor_position);
        QAccessible::updateAccessibility(&ev);
    }
}


// Convert bytes to text.
QString QsciAccessibleScintillaBase::bytesAsText(QsciScintillaBase *sb,
        const char *bytes, int length)
{
    if (sb->SendScintilla(QsciScintillaBase::SCI_GETCODEPAGE) == QsciScintillaBase::SC_CP_UTF8)
        return QString::fromUtf8(bytes, length);

    return QString::fromLatin1(bytes, length);
}


// Convert text to bytes.
QByteArray QsciAccessibleScintillaBase::textAsBytes(QsciScintillaBase *sb,
        const QString &text)
{
    if (sb->SendScintilla(QsciScintillaBase::SCI_GETCODEPAGE) == QsciScintillaBase::SC_CP_UTF8)
        return text.toUtf8();

    return text.toLatin1();
}


// Convert a byte position to a text position.
int QsciAccessibleScintillaBase::positionAsTextPosition(QsciScintillaBase *sb,
        int position)
{
    return sb->SendScintilla(QsciScintillaBase::SCI_POSITIONRELATIVE, 0,
            position);
}


// Convert a text position to a byte position.
int QsciAccessibleScintillaBase::textPositionAsPosition(QsciScintillaBase *sb,
        int textPosition)
{
    int position = 0;

    for (int p = 0; p < textPosition; ++p)
        position = sb->SendScintilla(QsciScintillaBase::SCI_POSITIONAFTER,
                position);

    return position;
}


// Get the current selection if any.
void QsciAccessibleScintillaBase::selection(int selectionIndex,
        int *startOffset, int *endOffset) const
{
    int start, end;

    if (selectionIndex == 0 && is_selection)
    {
        QsciScintillaBase *sb = sciWidget();
        int start_position = sb->SendScintilla(
                QsciScintillaBase::SCI_GETSELECTIONSTART);
        int end_position = sb->SendScintilla(
                QsciScintillaBase::SCI_GETSELECTIONEND);

        start = positionAsTextPosition(sb, start_position);
        end = positionAsTextPosition(sb, end_position);
    }
    else
    {
        start = end = 0;
    }

    *startOffset = start;
    *endOffset = end;
}


// Return the number of selections.
int QsciAccessibleScintillaBase::selectionCount() const
{
    return (is_selection ? 1 : 0);
}


// Add a selection.
void QsciAccessibleScintillaBase::addSelection(int startOffset, int endOffset)
{
    setSelection(0, startOffset, endOffset);
}


// Remove a selection.
void QsciAccessibleScintillaBase::removeSelection(int selectionIndex)
{
    if (selectionIndex == 0)
        sciWidget()->SendScintilla(QsciScintillaBase::SCI_CLEARSELECTIONS);
}


// Set the selection.
void QsciAccessibleScintillaBase::setSelection(int selectionIndex,
        int startOffset, int endOffset)
{
    if (selectionIndex == 0)
    {
        QsciScintillaBase *sb = sciWidget();
        sb->SendScintilla(QsciScintillaBase::SCI_SETSELECTIONSTART,
                textPositionAsPosition(sb, startOffset));
        sb->SendScintilla(QsciScintillaBase::SCI_SETSELECTIONEND,
                textPositionAsPosition(sb, endOffset));
    }
}


// Return the current cursor text position.
int QsciAccessibleScintillaBase::cursorPosition() const
{
    return current_cursor_position;
}


// Set the cursor position.
void QsciAccessibleScintillaBase::setCursorPosition(int position)
{
    QsciScintillaBase *sb = sciWidget();

    sb->SendScintilla(QsciScintillaBase::SCI_GOTOPOS,
            textPositionAsPosition(sb, position));
}


// Return the text between two positions.
QString QsciAccessibleScintillaBase::text(int startOffset, int endOffset) const
{
    QsciScintillaBase *sb = sciWidget();
    int byte_start = textPositionAsPosition(sb, startOffset);
    int byte_end = textPositionAsPosition(sb, endOffset);

    QByteArray bytes(byte_end - byte_start + 1, '\0');

    sb->SendScintilla(QsciScintillaBase::SCI_GETTEXTRANGE, byte_start,
            byte_end, bytes.data());

    return bytesAsText(sb, bytes.constData(), bytes.size() - 1);
}


// Return the number of characters in the text.
int QsciAccessibleScintillaBase::characterCount() const
{
    QsciScintillaBase *sb = sciWidget();

    return positionAsTextPosition(sb,
            sb->SendScintilla(QsciScintillaBase::SCI_GETTEXTLENGTH));
}


QRect QsciAccessibleScintillaBase::characterRect(int offset) const
{
    QsciScintillaBase *sb = sciWidget();
    int position = textPositionAsPosition(sb, offset);
    int x_vport = sb->SendScintilla(QsciScintillaBase::SCI_POINTXFROMPOSITION,
            position);
    int y_vport = sb->SendScintilla(QsciScintillaBase::SCI_POINTYFROMPOSITION,
            position);
    const QString ch = text(offset, offset + 1);

    // Get the character's font metrics.
    int style = sb->SendScintilla(QsciScintillaBase::SCI_GETSTYLEAT, position);
    QFontMetrics metrics(fontForStyle(style));

    QRect rect(x_vport, y_vport, metrics.width(ch), metrics.height());
    rect.moveTo(sb->viewport()->mapToGlobal(rect.topLeft()));

    return rect;
}


// Return the offset of the character at the given screen coordinates.
int QsciAccessibleScintillaBase::offsetAtPoint(const QPoint &point) const
{
    QsciScintillaBase *sb = sciWidget();
    QPoint p = sb->viewport()->mapFromGlobal(point);
    int position = sb->SendScintilla(QsciScintillaBase::SCI_POSITIONFROMPOINT,
            p.x(), p.y());

    return (position >= 0) ? positionAsTextPosition(sb, position) : -1;
}


// Scroll to make sure an area of text is visible.
void QsciAccessibleScintillaBase::scrollToSubstring(int startIndex,
        int endIndex)
{
    QsciScintillaBase *sb = sciWidget();
    int start = textPositionAsPosition(sb, startIndex);
    int end = textPositionAsPosition(sb, endIndex);

    sb->SendScintilla(QsciScintillaBase::SCI_SCROLLRANGE, end, start);
}


// Return the attributes of a character and surrounding text.
QString QsciAccessibleScintillaBase::attributes(int offset, int *startOffset,
        int *endOffset) const
{
    QsciScintillaBase *sb = sciWidget();
    int position = textPositionAsPosition(sb, offset);
    int style = sb->SendScintilla(QsciScintillaBase::SCI_GETSTYLEAT, position);

    // Find the start of the text with this style.
    int start_position = position;
    int start_text_position = offset;

    while (start_position > 0)
    {
        int before = sb->SendScintilla(QsciScintillaBase::SCI_POSITIONBEFORE,
                start_position);
        int s = sb->SendScintilla(QsciScintillaBase::SCI_GETSTYLEAT, before);

        if (s != style)
            break;

        start_position = before;
        --start_text_position;
    }

    *startOffset = start_text_position;

    // Find the end of the text with this style.
    int end_position = sb->SendScintilla(QsciScintillaBase::SCI_POSITIONAFTER,
            position);
    int end_text_position = offset + 1;
    int last_position = sb->SendScintilla(
            QsciScintillaBase::SCI_GETTEXTLENGTH);

    while (end_position < last_position)
    {
        int s = sb->SendScintilla(QsciScintillaBase::SCI_GETSTYLEAT,
                end_position);

        if (s != style)
            break;

        end_position = sb->SendScintilla(QsciScintillaBase::SCI_POSITIONAFTER,
                end_position);
        ++end_text_position;
    }

    *endOffset = end_text_position;

    // Convert the style to attributes.
    QString attrs;

    int back = sb->SendScintilla(QsciScintillaBase::SCI_STYLEGETBACK, style);
    addAttribute(attrs, "background-color", colourAsRGB(back));

    int fore = sb->SendScintilla(QsciScintillaBase::SCI_STYLEGETFORE, style);
    addAttribute(attrs, "color", colourAsRGB(fore));

    QFont font = fontForStyle(style);

    QString family = font.family();
    family = family.replace('\\', QLatin1String("\\\\"));
    family = family.replace(':', QLatin1String("\\:"));
    family = family.replace(',', QLatin1String("\\,"));
    family = family.replace('=', QLatin1String("\\="));
    family = family.replace(';', QLatin1String("\\;"));
    family = family.replace('\"', QLatin1String("\\\""));
    addAttribute(attrs, "font-familly",
            QLatin1Char('"') + family + QLatin1Char('"'));

    int font_size = int(font.pointSize());
    addAttribute(attrs, "font-size",
            QString::fromLatin1("%1pt").arg(font_size));

    QFont::Style font_style = font.style();
    addAttribute(attrs, "font-style",
            QString::fromLatin1((font_style == QFont::StyleItalic) ? "italic" : ((font_style == QFont::StyleOblique) ? "oblique": "normal")));

    int font_weight = font.weight();
    addAttribute(attrs, "font-weight",
            QString::fromLatin1(
                    (font_weight > QFont::Normal) ? "bold" : "normal"));

    int underline = sb->SendScintilla(QsciScintillaBase::SCI_STYLEGETUNDERLINE,
            style);
    if (underline)
        addAttribute(attrs, "text-underline-type",
                QString::fromLatin1("single"));

    return attrs;
}


// Add an attribute name/value pair.
void QsciAccessibleScintillaBase::addAttribute(QString &attrs,
        const char *name, const QString &value)
{
    attrs.append(QLatin1String(name));
    attrs.append(QChar(':'));
    attrs.append(value);
    attrs.append(QChar(';'));
}


// Convert a integer colour to an RGB string.
QString QsciAccessibleScintillaBase::colourAsRGB(int colour)
{
    return QString::fromLatin1("rgb(%1,%2,%3)").arg(colour & 0xff).arg((colour >> 8) & 0xff).arg((colour >> 16) & 0xff);
}


// Convert a integer colour to an RGB string.
QFont QsciAccessibleScintillaBase::fontForStyle(int style) const
{
    QsciScintillaBase *sb = sciWidget();
    char fontName[64];
    int len = sb->SendScintilla(QsciScintillaBase::SCI_STYLEGETFONT, style,
            fontName);
    int size = sb->SendScintilla(QsciScintillaBase::SCI_STYLEGETSIZE, style);
    bool italic = sb->SendScintilla(QsciScintillaBase::SCI_STYLEGETITALIC,
            style);
    int weight = sb->SendScintilla(QsciScintillaBase::SCI_STYLEGETWEIGHT,
            style);

    return QFont(QString::fromUtf8(fontName, len), size, weight, italic);
}


// Delete some text.
void QsciAccessibleScintillaBase::deleteText(int startOffset, int endOffset)
{
    addSelection(startOffset, endOffset);
    sciWidget()->SendScintilla(QsciScintillaBase::SCI_REPLACESEL, "");
}


// Insert some text.
void QsciAccessibleScintillaBase::insertText(int offset, const QString &text)
{
    QsciScintillaBase *sb = sciWidget();

    sb->SendScintilla(QsciScintillaBase::SCI_INSERTTEXT,
            textPositionAsPosition(sb, offset),
            textAsBytes(sb, text).constData());
}


// Replace some text.
void QsciAccessibleScintillaBase::replaceText(int startOffset, int endOffset,
        const QString &text)
{
    QsciScintillaBase *sb = sciWidget();

    addSelection(startOffset, endOffset);
    sb->SendScintilla(QsciScintillaBase::SCI_REPLACESEL,
            textAsBytes(sb, text).constData());
}


// Return the state.
QAccessible::State QsciAccessibleScintillaBase::state() const
{
    QAccessible::State st = QAccessibleWidget::state();

    st.selectableText = true;
    st.multiLine = true;

    if (sciWidget()->SendScintilla(QsciScintillaBase::SCI_GETREADONLY))
        st.readOnly = true;
    else
        st.editable = true;

    return st;
}


// Provide access to the indivual interfaces.
void *QsciAccessibleScintillaBase::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::TextInterface)
        return static_cast<QAccessibleTextInterface *>(this);

    if (t == QAccessible::EditableTextInterface)
        return static_cast<QAccessibleEditableTextInterface *>(this);

    return QAccessibleWidget::interface_cast(t);
}


// The accessibility interface factory.
static QAccessibleInterface *factory(const QString &classname, QObject *object)
{
    if (classname == QLatin1String("QsciScintillaBase") && object && object->isWidgetType())
        return new QsciAccessibleScintillaBase(static_cast<QWidget *>(object));

    return 0;
}


#endif
