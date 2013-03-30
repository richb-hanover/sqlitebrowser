#include "PreferencesDialog.h"
#include "ui_PreferencesDialog.h"
#include <QDir>
#include <QSettings>
#include <QFileDialog>
#include <QColorDialog>
#include "sqlitedb.h"

QHash<QString, QVariant> PreferencesDialog::m_hCache;

PreferencesDialog::PreferencesDialog(QWidget* parent)
    : QDialog(parent),
      ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);

    loadSettings();
}

/*
 *  Destroys the object and frees any allocated resources
 */
PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::chooseLocation()
{
    QString s = QFileDialog::getExistingDirectory(
                this,
                tr("Choose a directory"),
                getSettingsValue("db", "defaultlocation").toString(),
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if(!s.isEmpty())
        ui->locationEdit->setText(s);
}

void PreferencesDialog::loadSettings()
{
    ui->encodingComboBox->setCurrentIndex(ui->encodingComboBox->findText(getSettingsValue("db", "defaultencoding").toString(), Qt::MatchFixedString));
    ui->locationEdit->setText(getSettingsValue("db", "defaultlocation").toString());
    ui->foreignKeysCheckBox->setChecked(getSettingsValue("db", "foreignkeys").toBool());

    for(int i=0;i<ui->treeSyntaxHighlighting->topLevelItemCount();i++)
    {
        QString name;
        if(i == 0) name = "keyword";
        else if(i == 1) name = "table";
        else if(i == 2) name = "comment";
        else if(i == 3) name = "identifier";
        else if(i == 4) name = "string";

        ui->treeSyntaxHighlighting->topLevelItem(i)->setText(1, getSettingsValue("syntaxhighlighter", name + "_colour").toString());
        ui->treeSyntaxHighlighting->topLevelItem(i)->setCheckState(2, getSettingsValue("syntaxhighlighter", name + "_bold").toBool() ? Qt::Checked : Qt::Unchecked);
        ui->treeSyntaxHighlighting->topLevelItem(i)->setCheckState(3, getSettingsValue("syntaxhighlighter", name + "_italic").toBool() ? Qt::Checked : Qt::Unchecked);
        ui->treeSyntaxHighlighting->topLevelItem(i)->setCheckState(4, getSettingsValue("syntaxhighlighter", name + "_underline").toBool() ? Qt::Checked : Qt::Unchecked);
    }
}

void PreferencesDialog::saveSettings()
{
    setSettingsValue("db", "defaultencoding", ui->encodingComboBox->currentText());
    setSettingsValue("db", "defaultlocation", ui->locationEdit->text());
    setSettingsValue("db", "foreignkeys", ui->foreignKeysCheckBox->isChecked());

    for(int i=0;i<ui->treeSyntaxHighlighting->topLevelItemCount();i++)
    {
        QString name;
        if(i == 0) name = "keyword";
        else if(i == 1) name = "table";
        else if(i == 2) name = "comment";
        else if(i == 3) name = "identifier";
        else if(i == 4) name = "string";

        setSettingsValue("syntaxhighlighter", name + "_colour", ui->treeSyntaxHighlighting->topLevelItem(i)->text(1));
        setSettingsValue("syntaxhighlighter", name + "_bold", ui->treeSyntaxHighlighting->topLevelItem(i)->checkState(2) == Qt::Checked);
        setSettingsValue("syntaxhighlighter", name + "_italic", ui->treeSyntaxHighlighting->topLevelItem(i)->checkState(3) == Qt::Checked);
        setSettingsValue("syntaxhighlighter", name + "_underline", ui->treeSyntaxHighlighting->topLevelItem(i)->checkState(4) == Qt::Checked);
    }

    accept();
}

QVariant PreferencesDialog::getSettingsValue(const QString& group, const QString& name)
{
    // Have a look in the cache first
    QHash<QString, QVariant>::iterator cacheIndex = m_hCache.find(group + name);
    if(cacheIndex != m_hCache.end())
    {
        return cacheIndex.value();
    } else {
        // Nothing found in the cache, so get the value from the settings file or get the default value if there is no value set yet
        QSettings settings(QApplication::organizationName(), QApplication::organizationName());
        QVariant value = settings.value(group + "/" + name, getSettingsDefaultValue(group, name));

        // Store this value in the cache for further usage and return it afterwards
        m_hCache.insert(group + name, value);
        return value;
    }
}

void PreferencesDialog::setSettingsValue(const QString& group, const QString& name, const QVariant& value)
{
    // Set the group and save the given value
    QSettings settings(QApplication::organizationName(), QApplication::organizationName());
    settings.beginGroup(group);
    settings.setValue(name, value);
    settings.endGroup();

    // Also change it in the cache
    m_hCache[group + name] = value;
}

QVariant PreferencesDialog::getSettingsDefaultValue(const QString& group, const QString& name)
{
    // db/defaultencoding?
    if(group == "db" && name == "defaultencoding")
        return "UTF-8";

    // db/defaultlocation?
    if(group == "db" && name == "defaultlocation")
        return QDir::homePath();

    // db/foreignkeys?
    if(group == "db" && name == "foreignkeys")
        return true;

    // MainWindow/geometry?
    if(group == "MainWindow" && name == "geometry")
        return "";

    // MainWindow/windowState?
    if(group == "MainWindow" && name == "windowState")
        return "";

    // SQLLogDock/Log?
    if(group == "SQLLogDock" && name == "Log")
        return "Application";

    // General/recentFileList?
    if(group == "General" && name == "recentFileList")
        return QStringList();

    // syntaxhighlighter?
    if(group == "syntaxhighlighter")
    {
        // Bold? Only tables and keywords are bold by default
        if(name.right(4) == "bold")
            return name == "keyword_bold" || name == "table_bold";

        // Italic? Nothing by default
        if(name.right(6) == "italic")
            return false;

        // Underline? Nothing by default
        if(name.right(9) == "underline")
            return false;

        // Colour?
        if(name.right(6) == "colour")
        {
            if(name == "keyword_colour")
                return QColor(Qt::darkBlue).name();
            else if(name == "table_colour")
                return QColor(Qt::darkCyan).name();
            else if(name == "comment_colour")
                return QColor(Qt::darkGreen).name();
            else if(name == "identifier_colour")
                return QColor(Qt::darkMagenta).name();
            else if(name == "string_colour")
                return QColor(Qt::red).name();
        }
    }

    // Unknown combination of group and name? Return an invalid QVariant!
    return QVariant();
}

void PreferencesDialog::showColourDialog(QTreeWidgetItem* item, int column)
{
    if(item->text(column).left(1) != "#")
        return;

    QColor colour = QColorDialog::getColor(QColor(item->text(column)), this);
    if(colour.isValid())
        item->setText(column, colour.name());
}
