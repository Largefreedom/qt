/***************************************************************************
 *   This file is part of the CuteReport project                           *
 *   Copyright (C) 2012-2014 by Alexander Mikhalov                         *
 *   alexander.mikhalov@gmail.com                                          *
 *                                                                         *
 **                   GNU General Public License Usage                    **
 *                                                                         *
 *   This library is free software: you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 **                  GNU Lesser General Public License                    **
 *                                                                         *
 *   This library is free software: you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License as        *
 *   published by the Free Software Foundation, either version 3 of the    *
 *   License, or (at your option) any later version.                       *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library.                                      *
 *   If not, see <http://www.gnu.org/licenses/>.                           *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 ****************************************************************************/

#include "reportinterface.h"
#include "pageinterface.h"
#include "iteminterface.h"
#include "datasetinterface.h"
#include "forminterface.h"
#include "rendererinterface.h"
#include "printerinterface.h"
#include "storageinterface.h"
#include "log/log.h"
#include "globals.h"
#include "renderedreportinterface.h"
#include "reportcore.h"

using namespace CuteReport;


ReportInterface::ReportInterface(QObject *parent)
    : QObject(parent),
      //      m_reportCore(0),
      m_version(0.0),
      m_isDirty(false),
      m_isValid(true),
      m_renderedReport(0)
{
    Log::refCounterInc(this);
}


ReportInterface::ReportInterface(const ReportInterface & dd, QObject * parent)
    :QObject(parent),
      //      m_reportCore(dd.m_reportCore),
      m_name(dd.m_name),
      m_author(dd.m_author),
      m_script(dd.m_script),
      m_version(dd.m_version),
      m_description(dd.m_description),
      m_filePath(dd.m_filePath),
      m_defaultStorageName(dd.m_defaultStorageName),
      m_variables(dd.m_variables),
      m_flags(0),
      m_isDirty(dd.m_isDirty),
      m_isValid(dd.m_isValid),
      m_renderedReport(0)
{
    Log::refCounterInc(this);
    setFlags(dd.m_flags);
}


ReportInterface::~ReportInterface()
{
    delete m_renderedReport;
    Log::refCounterDec(this);
}


void ReportInterface::init()
{
    QList<CuteReport::ReportPluginInterface*> modules = this->findChildren<CuteReport::ReportPluginInterface*>();
    qDebug() << modules.count();
    QList<CuteReport::PageInterface*> pages = this->findChildren<CuteReport::PageInterface*>();
    foreach(CuteReport::PageInterface* page, pages) {
        page->init();
        connect(page, SIGNAL(destroyed(QObject*)), this, SLOT(childDestroyed(QObject*)));
        connect(page, SIGNAL(afterNewItemAdded(CuteReport::BaseItemInterface*)),
                this, SLOT(slotItemAdded(CuteReport::BaseItemInterface*)));
        connect(page, SIGNAL(afterItemRemoved(CuteReport::BaseItemInterface*,QString,bool)),
                this, SLOT(slotItemRemoved(CuteReport::BaseItemInterface*, QString, bool)));
    }

    //    QList<CuteReport::BaseItemInterface*> items = this->findChildren<CuteReport::BaseItemInterface*>();
    //    foreach(CuteReport::BaseItemInterface* item, items) {
    //        item->init();
    //    }

    QList<CuteReport::DatasetInterface*> datasets = this->findChildren<CuteReport::DatasetInterface*>();
    foreach(CuteReport::DatasetInterface* dataset, datasets) {
        dataset->init();
        connect(dataset, SIGNAL(destroyed(QObject*)), this, SLOT(childDestroyed(QObject*)));
    }

    QList<CuteReport::FormInterface*> forms = this->findChildren<CuteReport::FormInterface*>();
    foreach(CuteReport::FormInterface* form, forms) {
        form->init();
        connect(form, SIGNAL(destroyed(QObject*)), this, SLOT(childDestroyed(QObject*)));
    }

    precessFlags(m_flags);
}


ReportInterface * ReportInterface::clone(bool withChildren, bool init)
{
    ReportInterface * newReport = new ReportInterface(*this, this->parent());
    if (withChildren) {
        foreach (QObject * child, children()) {
            if (PageInterface * page = dynamic_cast<PageInterface*>(child)) {
                PageInterface * newPage = page->clone(true, false);
                newPage->setParent(newReport);
                newPage->setObjectName(page->objectName());
            }
            if (DatasetInterface * ds = dynamic_cast<DatasetInterface*>(child)) {
                DatasetInterface * newds = ds->clone();
                newds->setParent(newReport);
                newds->setObjectName(ds->objectName());
            }
            if (FormInterface * form = dynamic_cast<FormInterface*>(child)) {
                FormInterface * newForm = form->clone();
                newForm->setParent(newReport);
                newForm->setObjectName(form->objectName());
            }
            if (RendererInterface * renderer = dynamic_cast<RendererInterface*>(child)) {
                RendererInterface * newRenderer = renderer->clone();
                newRenderer->setParent(newReport);
                newRenderer->setObjectName(renderer->objectName());
            }
            if (PrinterInterface * printer = dynamic_cast<PrinterInterface*>(child)) {
                PrinterInterface * newPrinter = printer->clone();
                newPrinter->setParent(newReport);
                newPrinter->setObjectName(printer->objectName());
            }
            if (StorageInterface * storage = dynamic_cast<StorageInterface*>(child)) {
                StorageInterface * newStorage = storage->clone();
                newStorage->setParent(newReport);
                newStorage->setObjectName(storage->objectName());
            }
        }
    }

    if (init)
        newReport->init();

    return newReport;
}


QString ReportInterface::script()
{
    return m_script;
}


void ReportInterface::setScript(const QString & script)
{
    if (m_script == script)
        return;

    m_script = script;

    emit scriptChanged(m_script);
    emit changed();
    emit propertyChanged();
}


QString ReportInterface::name()
{
    return m_name;
}


void ReportInterface::setName(const QString & name)
{
    if (m_name == name)
        return;
    m_name = name;

    emit nameChanged(m_name);
    emit changed();
    emit propertyChanged();
}


QString ReportInterface::author()
{
    return m_author;
}


void ReportInterface::setAuthor(const QString & author)
{
    if (m_author == author)
        return;
    m_author = author;

    emit authorChanged(m_author);
    emit changed();
    emit propertyChanged();
}


QList<BaseItemInterface *> ReportInterface::items(const QString &pageObjectName)
{
    if (pageObjectName.isEmpty())
        return this->findChildren<CuteReport::BaseItemInterface*>();

    const PageInterface * p = page(pageObjectName);
    if (!p)
        return QList<BaseItemInterface *>();

    return p->items();
}


CuteReport::BaseItemInterface * ReportInterface::item (const QString & itemName)
{
    foreach (PageInterface* page, pages())
        foreach (BaseItemInterface * item, page->items()) {
            if (item->objectName() == itemName) {
                return item;
            }
        }
    return 0;
}


QList<PageInterface *> ReportInterface::pages()
{
    return this->findChildren<CuteReport::PageInterface*>();
}


QStringList ReportInterface::pageNames()
{
    QStringList list;
    foreach (PageInterface * page, pages())
        list << page->objectName();
    return list;
}


PageInterface *ReportInterface::page(const QString &pageName)
{
    if (pageName.isEmpty())
        return 0;
    return findChild<PageInterface *>(pageName);
}


void ReportInterface::addPage (PageInterface * page)
{
    page->setParent(this);
    setUniqueName(page);

    connect(page, SIGNAL(destroyed(QObject*)), this, SLOT(childDestroyed(QObject*)));
    connect(page, SIGNAL(afterNewItemAdded(CuteReport::BaseItemInterface*)),
            this, SLOT(slotItemAdded(CuteReport::BaseItemInterface*)));
    connect(page, SIGNAL(afterItemRemoved(CuteReport::BaseItemInterface*,QString,bool)),
            this, SLOT(slotItemRemoved(CuteReport::BaseItemInterface*, QString, bool)));

    if (m_flags.testFlag(DirtynessAutoUpdate))
        connect(page, SIGNAL(changed()), this, SLOT(setDirty()));

    emit pageAdded(page);
    emit changed();
}


void ReportInterface::deletePage(PageInterface * page)
{
    if (!page || page->parent() != this)
        return;

    /** disconnect for prevent triggering destroyed object postprocessing */
    disconnect(page, 0, this, 0);
    page->deleteLater();

    emit pageDeleted(page);
    emit changed();
}


void ReportInterface::deletePage(const QString &pageName)
{
    deletePage(page(pageName));
}


QList<DatasetInterface *> ReportInterface::datasets()
{
    return this->findChildren<CuteReport::DatasetInterface*>();
}


QStringList ReportInterface::datasetNames()
{
    QStringList list;
    foreach (DatasetInterface * ds, datasets())
        list << ds->objectName();
    return list;
}


DatasetInterface * ReportInterface::dataset(const QString & datasetName)
{
    if (datasetName.isEmpty())
        return 0;
    return findChild<DatasetInterface *>(datasetName);
}


void ReportInterface::addDatasets(QList<DatasetInterface *> datasets)
{
    foreach (DatasetInterface * dataset, datasets) {
        addDataset(dataset);
    }
}


void ReportInterface::addDataset(DatasetInterface* dataset)
{
    dataset->setParent(this);
    setUniqueName(dataset);

    connect(dataset, SIGNAL(destroyed(QObject*)), this, SLOT(childDestroyed(QObject*)));
    if (m_flags.testFlag(VariablesAutoUpdate))
        connect(dataset, SIGNAL(variablesChanged()), this, SLOT(slotObjectVariablesChanged()), Qt::UniqueConnection);
    if (m_flags.testFlag(DirtynessAutoUpdate))
        connect(dataset, SIGNAL(changed()), this, SLOT(setDirty()));

    emit datasetAdded(dataset);
    emit changed();
}


void ReportInterface::deleteDataset(DatasetInterface* dataset)
{
    if (!dataset)
        return;

    /** disconnect for prevent triggering destroyed object postprocessing */
    disconnect(dataset, 0, this, 0);

    QString name = dataset->objectName();
    delete dataset;

    emit datasetDeleted(dataset);
    emit datasetDeleted(name);
    emit changed();
}


void ReportInterface::deleteDataset(const QString &datasetName)
{
    deleteDataset(dataset(datasetName));
}


QList<FormInterface*> ReportInterface::forms()
{
    return findChildren<FormInterface *>();
}


FormInterface* ReportInterface::form(const QString & formName)
{
    return findChild<FormInterface *>(formName);
}


void ReportInterface::addForm(FormInterface * form )
{
    form->setParent(this);

    connect(form, SIGNAL(destroyed(QObject*)), this, SLOT(childDestroyed(QObject*)));
    if (m_flags.testFlag(VariablesAutoUpdate))
        connect(form, SIGNAL(scriptingStringsChanged()), this, SLOT(slotScriptStringsChanged()), Qt::UniqueConnection);

    emit formAdded(form);
    emit changed();
}


void ReportInterface::deleteForm(FormInterface * form )
{
    /** disconnect for prevent triggering destroyed object postprocessing */
    disconnect(form, 0, this, 0);
    delete form;

    emit formDeleted(form);
    emit changed();
}


QList<RendererInterface *> ReportInterface::renderers()
{
    return this->findChildren<CuteReport::RendererInterface*>();
}


QStringList ReportInterface::rendererNames()
{
    QStringList list;
    foreach (RendererInterface * rend, renderers())
        list << rend->objectName();
    return list;
}


RendererInterface * ReportInterface::renderer(const QString & rendererName)
{
    QString name = rendererName.isEmpty() ? m_defaultRendererName : rendererName;
    return findChild<CuteReport::RendererInterface*>(name);
}


void ReportInterface::addRenderer(CuteReport::RendererInterface * renderer)
{
    renderer->setParent(this);
    setUniqueName(renderer);

    connect(renderer, SIGNAL(destroyed(QObject*)), this, SLOT(childDestroyed(QObject*)));
    if (m_flags.testFlag(DirtynessAutoUpdate))
        connect(renderer, SIGNAL(changed()), this, SLOT(setDirty()));

    emit rendererAdded(renderer);
    emit changed();
}


void ReportInterface::deleteRenderer(RendererInterface *renderer)
{
    if (!renderer || renderer->parent() != this)
        return;

    /** disconnect for prevent triggering destroyed object postprocessing */
    disconnect(renderer, 0, this, 0);

    QString name = renderer->objectName();
    delete renderer;

    emit rendererDeleted(renderer);
    emit rendererDeleted(name);
    emit changed();
}


void ReportInterface::deleteRenderer(const QString &rendererName)
{
    deleteRenderer(renderer(rendererName));
}


QString ReportInterface::defaultRendererName() const
{
    return m_defaultRendererName;
}


void ReportInterface::setDefaultRendererName(const QString &name)
{
    if (m_defaultRendererName == name)
        return;

    m_defaultRendererName = name;

    emit defaultRendererChanged(m_defaultRendererName);
    emit changed();
    emit propertyChanged();
}


QList<PrinterInterface *> ReportInterface::printers()
{
    return this->findChildren<CuteReport::PrinterInterface*>();
}


QStringList ReportInterface::printerNames()
{
    QStringList list;
    foreach (PrinterInterface * printer, printers())
        list << printer->objectName();
    return list;
}


PrinterInterface * ReportInterface::printer(const QString & printerName)
{
    return findChild<CuteReport::PrinterInterface*>(printerName);
}


void ReportInterface::addPrinter(PrinterInterface *printer)
{
    printer->setParent(this);
    setUniqueName(printer);

    connect(printer, SIGNAL(destroyed(QObject*)), this, SLOT(childDestroyed(QObject*)));
    if (m_flags.testFlag(DirtynessAutoUpdate))
        connect(printer, SIGNAL(changed()), this, SLOT(setDirty()));

    emit printerAdded(printer);
    emit changed();
}


void ReportInterface::deletePrinter(PrinterInterface *printer)
{
    if (!printer || printer->parent() != this)
        return;

    /** disconnect for prevent triggering destroyed object postprocessing */
    disconnect(printer, 0, this, 0);

    QString name = printer->objectName();
    delete printer;

    emit printerDeleted(printer);
    emit printerDeleted(name);
    emit changed();
}


void ReportInterface::deletePrinter(const QString &printerName)
{
    deletePrinter(printer(printerName));
}


QString ReportInterface::defaultPrinterName() const
{
    return m_defaultPrinterName;
}


void ReportInterface::setDefaultPrinterName(const QString &name)
{
    if (m_defaultPrinterName == name)
        return;

    m_defaultPrinterName = name;

    emit defaultPrinterChanged(m_defaultPrinterName);
    emit changed();
    emit propertyChanged();
}


StorageInterface *ReportInterface::storage(const QString &objectName) const
{
    StorageInterface * module = 0;
    foreach (StorageInterface * m, findChildren<CuteReport::StorageInterface*>()) {
        if (m->objectName() == objectName) {
            module = m;
            break;
        }
    }

    return module;
}


StorageInterface *ReportInterface::storageByUrl(const QString &url) const
{
    QString storageName = url.section(':', 0,0);
    return storage(storageName);
}


QList<StorageInterface *> ReportInterface::storageListByScheme(const QString & scheme) const
{
    QList<StorageInterface *> resultList;
    foreach (StorageInterface * m, findChildren<CuteReport::StorageInterface*>()) {
        if (m->urlScheme() == scheme)
            resultList << m;
    }
    return resultList;
}


QList<StorageInterface *> ReportInterface::storageListByModuleName(const QString & moduleName) const
{
    QList<StorageInterface *> resultList;
    foreach (StorageInterface * m, findChildren<CuteReport::StorageInterface*>()) {
        if (m->moduleFullName() == moduleName)
            resultList << m;
    }
    return resultList;
}


QList<StorageInterface *> ReportInterface::storages() const
{
    return findChildren<CuteReport::StorageInterface*>();
}


QStringList ReportInterface::storageNames() const
{
    QStringList list;
    foreach (StorageInterface * storage, storages())
        list << storage->objectName();
    return list;
}


void ReportInterface::addStorage(StorageInterface *storage)
{
    if (!storage || storage->parent() == this)
        return;

    storage->setParent(this);
    storage->setObjectName( ReportCore::uniqueName(storage, storage->urlScheme(), this) );

    emit storageAdded(storage);
    emit changed();
}


void ReportInterface::deleteStorage(const QString & storageName)
{
    deleteStorage(storage(storageName));
}


void ReportInterface::deleteStorage(StorageInterface * storage)
{
    if (!storage || storage->parent() != this)
        return;

//    if (defaultStorageName() == storage->objectName())
//        setDefaultStorageName(QString());

    QString name = storage->objectName();
    delete storage;

    emit storageDeleted(storage);
    emit storageDeleted(name);
    emit changed();
}


bool ReportInterface::hasStorageModule(const QString & moduleName)
{
    QList<CuteReport::StorageInterface*> storages = findChildren<CuteReport::StorageInterface*>();
    CuteReport::StorageInterface* existentStorage = 0;
    foreach(CuteReport::StorageInterface* st, storages)
        if (moduleName == st->moduleFullName()) {
            existentStorage = st;
            break;
        }

    return existentStorage;
}


QString ReportInterface::defaultStorageName() const
{
    return m_defaultStorageName;
}


void ReportInterface::setDefaultStorageName(const QString & name)
{
    if (m_defaultStorageName == name)
        return;

    m_defaultStorageName = name;

    emit defaultStorageChanged(m_defaultStorageName);
    emit changed();
    emit propertyChanged();
}



// FIXMI: not imlemented report file version checking
double ReportInterface::version()
{
    return m_version;
}


void ReportInterface::setVersion(double tVersion)
{
    m_version = tVersion;
}


int ReportInterface::suitId()
{
    return 0;
}


QString ReportInterface::description()
{
    return m_description;
}


void ReportInterface::setDescription(const QString & description)
{
    if (m_description == description)
        return;

    m_description = description;

    emit descriptionChanged(m_description);
    emit changed();
    emit propertyChanged();
}


QString ReportInterface::fileUrl()
{
    return m_filePath;
}


void ReportInterface::setFilePath(const QString & filePath)
{
    if (filePath == m_filePath)
        return;

    m_filePath = filePath;

    emit fileUrlChanged(m_filePath);
    emit changed();
    emit propertyChanged();
}


const QVariantHash &ReportInterface::variables()
{
    return m_variables;
}


void ReportInterface::setVariables (const QVariantHash &vars)
{
    if (m_variables == vars)
        return;

    m_variables = vars;

    emit variablesChanged();
    emit changed();
}


void ReportInterface::setVariableValue(const QString & name, const QVariant & value)
{
    m_variables.insert(name, value);
}


void ReportInterface::updateVariables()
{
    QVariantHash oldVariables = m_variables;
    m_variables.clear();

    QSet<QString> set;

    set.unite( findVariables(m_script) );

    foreach (PageInterface * page, pages()) {
        foreach (BaseItemInterface * item, page->items()) {
            QStringList scriptingStrings = item->scriptingStrings();
            foreach (const QString & str, scriptingStrings) {
                set.unite( findVariables(str) );
            }
        }
    }

    foreach (DatasetInterface * ds, datasets()) {
        set.unite(ds->variables());
    }

    foreach (FormInterface * form, forms()) {
        QStringList scriptingStrings = form->scriptingStrings();
        foreach (const QString & str, scriptingStrings) {
            set.unite( findVariables(str) );
        }
    }

    foreach (const QString &key, set)
        m_variables.insert(key, oldVariables.value(key));

    variablesChanged();
}


void ReportInterface::childDestroyed(QObject * object)
{
    if (qobject_cast<CuteReport::DatasetInterface*>(object))
        emit datasetDeleted((CuteReport::DatasetInterface*) object);
    else if (qobject_cast<CuteReport::FormInterface*>(object))
        emit formDeleted((CuteReport::FormInterface*) object);
    else if (qobject_cast<CuteReport::PageInterface*>(object))
        emit pageDeleted((CuteReport::PageInterface*) object);
}


void ReportInterface::slotItemAdded(CuteReport::BaseItemInterface*item)
{
    emit itemAdded(item);
}


void ReportInterface::slotItemRemoved(CuteReport::BaseItemInterface* item, QString, bool directDeletion)
{
    emit itemDeleted(item, directDeletion);
}


void ReportInterface::setFlag(ReportFlag flag, bool enable)
{
    ReportFlags prev = m_flags;
    if (enable)
        m_flags |= flag;
    else
        m_flags &= ~flag;
    precessFlags(prev);
}


void ReportInterface::setFlags(ReportFlags flags)
{
    ReportFlags prev = m_flags;
    m_flags = flags;
    precessFlags(prev);
}


bool ReportInterface::isFlagSet(ReportFlag flag)
{
    return m_flags.testFlag(flag);
}


ReportInterface::ReportFlags ReportInterface::flags()
{
    return m_flags;
}


void ReportInterface::precessFlags(ReportFlags previousFlags)
{
    if (m_flags.testFlag(VariablesAutoUpdate) && !previousFlags.testFlag(VariablesAutoUpdate)) {

        foreach (PageInterface * page, pages()) {
            foreach (BaseItemInterface * item, page->items())
                connect(item, SIGNAL(scriptingStringsChanged()), this, SLOT(slotScriptStringsChanged()), Qt::UniqueConnection);
        }
        foreach (DatasetInterface * dataset, datasets())
            connect(dataset, SIGNAL(variablesChanged()), this, SLOT(slotObjectVariablesChanged()), Qt::UniqueConnection);
        foreach (FormInterface * form, forms())
            connect(form, SIGNAL(scriptingStringsChanged()), this, SLOT(slotScriptStringsChanged()), Qt::UniqueConnection);

        connect(this, SIGNAL(scriptChanged(QString)), this, SLOT(slotScriptStringsChanged()), Qt::UniqueConnection);
    }

    if (!m_flags.testFlag(VariablesAutoUpdate) && previousFlags.testFlag(VariablesAutoUpdate)) {

        foreach (PageInterface * page, pages()) {
            foreach (BaseItemInterface * item, page->items())
                disconnect(item, SIGNAL(scriptingStringsChanged()), this, SLOT(slotScriptStringsChanged()));
        }
        foreach (DatasetInterface * dataset, datasets())
            disconnect(dataset, SIGNAL(variablesChanged()), this, SLOT(slotObjectVariablesChanged()));
        foreach (FormInterface * form, forms())
            disconnect(form, SIGNAL(scriptingStringsChanged()), this, SLOT(slotScriptStringsChanged()));

        disconnect(this, SIGNAL(scriptChanged(QString)), this, SLOT(slotScriptStringsChanged()));
    }

    if (m_flags.testFlag(DirtynessAutoUpdate) && !previousFlags.testFlag(DirtynessAutoUpdate)) {
        foreach (PageInterface * page, pages()) {
            connect(page, SIGNAL(afterNewItemAdded(CuteReport::BaseItemInterface*)),
                    this, SLOT(slotNewItemAdded(CuteReport::BaseItemInterface*)), Qt::UniqueConnection);
            foreach (BaseItemInterface * item, page->items())
                connect(item, SIGNAL(changed()), this, SLOT(setDirty()), Qt::UniqueConnection);
        }
        foreach (DatasetInterface * dataset, datasets())
            connect(dataset, SIGNAL(changed()), this, SLOT(setDirty()), Qt::UniqueConnection);
        foreach (FormInterface * form, forms())
            connect(form, SIGNAL(changed()), this, SLOT(setDirty()), Qt::UniqueConnection);

        connect(this, SIGNAL(changed()), this, SLOT(setDirty()), Qt::UniqueConnection);
    }

    if (!m_flags.testFlag(DirtynessAutoUpdate) && previousFlags.testFlag(DirtynessAutoUpdate)) {
        foreach (PageInterface * page, pages()) {
            disconnect(page, SIGNAL(afterNewItemAdded(CuteReport::BaseItemInterface*,QPointF)),
                       this, SLOT(slotNewItemAdded(CuteReport::BaseItemInterface*)));
            foreach (BaseItemInterface * item, page->items())
                disconnect(item, SIGNAL(changed()), this, SLOT(setDirty()));
        }
        foreach (DatasetInterface * dataset, datasets())
            disconnect(dataset, SIGNAL(changed()), this, SLOT(setDirty()));
        foreach (FormInterface * form, forms())
            disconnect(form, SIGNAL(changed()), this, SLOT(setDirty()));

        disconnect(this, SIGNAL(changed()), this, SLOT(setDirty()));
    }
}


void ReportInterface::setUniqueName(QObject *object)
{
    if (ReportCore::isNameUnique(object, object->objectName(), this))
        return;
    object->setObjectName(ReportCore::uniqueName(object, object->objectName(), this));
}


void ReportInterface::slotScriptStringsChanged()
{
    updateVariables();

    emit variablesChanged();
    emit changed();
}


void ReportInterface::slotObjectVariablesChanged()
{
    updateVariables();

    emit variablesChanged();
    emit changed();
}


bool ReportInterface::isDirty() const
{
    return m_isDirty;
}


void ReportInterface::setDirty(bool b)
{
    if (m_isDirty == b)
        return;

    m_isDirty = b;

    emit dirtynessChanged(m_isDirty);
}


bool ReportInterface::isValid() const
{
    return m_isValid;
}


void ReportInterface::setValid(bool b)
{
    if (m_isValid == b)
        return;

    m_isValid = b;

    emit validityChanged(m_isValid);
}


void ReportInterface::setInvalid()
{
    if (!m_isValid)
        return;

    m_isValid = false;

    emit validityChanged(m_isValid);
}


void ReportInterface::slotNewItemAdded(CuteReport::BaseItemInterface *item)
{
    if (m_flags.testFlag(DirtynessAutoUpdate)) {
        connect(item, SIGNAL(changed()), this, SLOT(setDirty()), Qt::UniqueConnection);
    }
}


RenderedReportInterface *ReportInterface::renderedReport() const
{
    return m_renderedReport;
}


void ReportInterface::setRenderedReport(RenderedReportInterface *renderedReport)
{
    delete m_renderedReport;
    m_renderedReport = renderedReport;
}


void ReportInterface::clearRenderedReport()
{
    delete m_renderedReport;
    m_renderedReport = 0;
}
