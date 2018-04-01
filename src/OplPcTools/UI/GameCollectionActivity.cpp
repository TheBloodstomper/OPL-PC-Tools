/***********************************************************************************************
 *                                                                                             *
 * This file is part of the OPL PC Tools project, the graphical PC tools for Open PS2 Loader.  *
 *                                                                                             *
 * OPL PC Tools is free software: you can redistribute it and/or modify it under the terms of  *
 * the GNU General Public License as published by the Free Software Foundation,                *
 * either version 3 of the License, or (at your option) any later version.                     *
 *                                                                                             *
 * OPL PC Tools is distributed in the hope that it will be useful,  but WITHOUT ANY WARRANTY;  *
 * without even the implied warranty of  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  *
 * See the GNU General Public License for more details.                                        *
 *                                                                                             *
 * You should have received a copy of the GNU General Public License along with MailUnit.      *
 * If not, see <http://www.gnu.org/licenses/>.                                                 *
 *                                                                                             *
 ***********************************************************************************************/

#include <QFileDialog>
#include <QAbstractItemModel>
#include <OplPcTools/Core/Settings.h>
#include <OplPcTools/Core/GameCollection.h>
#include <OplPcTools/UI/Application.h>
#include <OplPcTools/UI/GameDetailsActivity.h>
#include <OplPcTools/UI/IsoRestorerActivity.h>
#include <OplPcTools/UI/GameCollectionActivity.h>
#include <OplPcTools/UI/GameInstallerActivity.h>

using namespace OplPcTools;
using namespace OplPcTools::UI;

namespace {

namespace SettingsKey {

const char * ul_dir     = "ULDirectory";
const char * icons_size = "GameListIconSize";

} // namespace SettingsKey

class GameCollectionActivityIntent : public Intent
{
public:
    Activity * createActivity(QWidget * _parent) override
    {
        return new GameCollectionActivity(_parent);
    }
};

} // namespace

class GameCollectionActivity::GameTreeModel : public QAbstractItemModel
{
public:
    explicit GameTreeModel(Core::GameCollection & _collection, QObject * _parent = nullptr);
    QModelIndex index(int _row, int _column, const QModelIndex & _parent) const override;
    QModelIndex parent(const QModelIndex & _child) const override;
    int rowCount(const QModelIndex & _parent) const override;
    int columnCount(const QModelIndex & _parent) const override;
    QVariant data(const QModelIndex & _index, int _role) const override;
    const Core::Game * game(const QModelIndex & _index) const;
    void setArtManager(Core::GameArtManager & _manager);

private:
    void collectionLoaded();
    void gameAdded(const QString & _id);
    void updateRecord(const QString & _id);
    void gameArtChanged(const QString & _game_id, Core::GameArtType _type, const QPixmap * _pixmap);

private:
    const QPixmap m_default_icon;
    const Core::GameCollection & mr_collection;
    Core::GameArtManager * mp_art_manager;
};


GameCollectionActivity::GameTreeModel::GameTreeModel(Core::GameCollection & _collection, QObject * _parent /*= nullptr*/) :
    QAbstractItemModel(_parent),
    m_default_icon(QPixmap(":/images/no-icon")),
    mr_collection(_collection),
    mp_art_manager(nullptr)
{
    connect(&_collection, &Core::GameCollection::loaded, this, &GameCollectionActivity::GameTreeModel::collectionLoaded);
    connect(&_collection, &Core::GameCollection::gameRenamed, this, &GameCollectionActivity::GameTreeModel::updateRecord);
    connect(&_collection, &Core::GameCollection::gameAdded, this, &GameCollectionActivity::GameTreeModel::gameAdded);
}

void GameCollectionActivity::GameTreeModel::collectionLoaded()
{
    beginResetModel();
    endResetModel();
}

void GameCollectionActivity::GameTreeModel::gameAdded(const QString & _id)
{
    Q_UNUSED(_id);
    beginResetModel();
    endResetModel();
}

void GameCollectionActivity::GameTreeModel::updateRecord(const QString & _id)
{
    int count = mr_collection.count();
    for(int i = 0; i < count; ++i)
    {
        const Core::Game * game = mr_collection[i];
        if(game->id() == _id)
        {
            emit dataChanged(createIndex(i, 0), createIndex(i, 0));
            return;
        }
    }
}

void GameCollectionActivity::GameTreeModel::gameArtChanged(const QString & _game_id, Core::GameArtType _type, const QPixmap * _pixmap)
{
    Q_UNUSED(_pixmap)
    if(_type == Core::GameArtType::Icon)
        updateRecord(_game_id);
}

QModelIndex GameCollectionActivity::GameTreeModel::index(int _row, int _column, const QModelIndex & _parent) const
{
    Q_UNUSED(_parent)
    return createIndex(_row, _column);
}

QModelIndex GameCollectionActivity::GameTreeModel::parent(const QModelIndex & _child) const
{
    Q_UNUSED(_child)
    return QModelIndex();
}

int GameCollectionActivity::GameTreeModel::rowCount(const QModelIndex & _parent) const
{
    if(_parent.isValid())
        return 0;
    return mr_collection.count();
}

int GameCollectionActivity::GameTreeModel::columnCount(const QModelIndex & _parent) const
{
    Q_UNUSED(_parent)
    return 1;
}

QVariant GameCollectionActivity::GameTreeModel::data(const QModelIndex & _index, int _role) const
{
    switch(_role)
    {
    case Qt::DisplayRole:
        return mr_collection[_index.row()]->title();
    case Qt::DecorationRole:
        if(mp_art_manager)
        {
            QPixmap icon = mp_art_manager->load(mr_collection[_index.row()]->id(), Core::GameArtType::Icon);
            return QIcon(icon.isNull() ? m_default_icon : icon);
        }
        break;
    }
    return QVariant();
}

const Core::Game * GameCollectionActivity::GameTreeModel::game(const QModelIndex & _index) const
{
    return _index.isValid() ? mr_collection[_index.row()] : nullptr;
}

void GameCollectionActivity::GameTreeModel::setArtManager(Core::GameArtManager & _manager)
{
    if(mp_art_manager)
        disconnect(mp_art_manager, &Core::GameArtManager::artChanged, this, &GameTreeModel::gameArtChanged);
    mp_art_manager = &_manager;
    connect(mp_art_manager, &Core::GameArtManager::artChanged, this, &GameTreeModel::gameArtChanged);
}

GameCollectionActivity::GameCollectionActivity(QWidget * _parent /*= nullptr*/) :
    Activity(_parent),
    mp_game_art_manager(nullptr),
    mp_model(nullptr),
    mp_proxy_model(nullptr)
{
    setupUi(this);
    m_default_cover = QPixmap(":/images/no-image")
        .scaled(mp_label_cover->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    Core::GameCollection & game_collection = Application::instance().gameCollection();
    mp_model = new GameTreeModel(game_collection, this);
    mp_proxy_model = new QSortFilterProxyModel(this);
    mp_proxy_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
    mp_proxy_model->setSourceModel(mp_model);
    mp_proxy_model->setDynamicSortFilter(true);
    mp_tree_games->setModel(mp_proxy_model);
    activateCollectionControls(false);
    activateItemControls(nullptr);
    connect(mp_tree_games->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(gameSelected()));
    connect(&game_collection, &Core::GameCollection::loaded, this, &GameCollectionActivity::collectionLoaded);
    connect(&game_collection, &Core::GameCollection::gameRenamed, this, &GameCollectionActivity::gameRenamed);
    connect(this, &GameCollectionActivity::destroyed, this, &GameCollectionActivity::saveSettings);
    connect(mp_edit_filter, &QLineEdit::textChanged, mp_proxy_model, &QSortFilterProxyModel::setFilterFixedString);
    connect(mp_btn_restore_iso, &QToolButton::clicked, this, &GameCollectionActivity::showIsoRestorer);
    applySettings();
}

QSharedPointer<Intent> GameCollectionActivity::createIntent()
{
    return QSharedPointer<Intent>(new GameCollectionActivityIntent);
}

bool GameCollectionActivity::onAttach()
{
    if(Core::Settings::instance().reopenLastSestion())
        tryLoadRecentDirectory();
    return true;
}

void GameCollectionActivity::activateCollectionControls(bool _activate)
{
    mp_btn_install->setEnabled(_activate);
    mp_btn_reload->setEnabled(_activate);
}

void GameCollectionActivity::activateItemControls(const Core::Game * _selected_game)
{
    mp_widget_details->setVisible(_selected_game);
    mp_btn_delete->setEnabled(_selected_game);
    mp_btn_edit->setEnabled(_selected_game);
    mp_btn_restore_iso->setEnabled(_selected_game && _selected_game->installationType() == Core::GameInstallationType::UlConfig);
}

void GameCollectionActivity::applySettings()
{
    QSettings settings;
    QVariant icons_size_value = settings.value(SettingsKey::icons_size);
    int icons_size = 3;
    if(!icons_size_value.isNull() && icons_size_value.canConvert(QVariant::Int))
    {
        icons_size = icons_size_value.toInt();
        if(icons_size > 4) icons_size = 4;
        else if(icons_size < 1) icons_size = 1;
    }
    mp_slider_icons_size->setValue(icons_size);
    icons_size *= 16;
    mp_tree_games->setIconSize(QSize(icons_size, icons_size));
}

void GameCollectionActivity::saveSettings()
{
    QSettings settings;
    settings.setValue(SettingsKey::icons_size, mp_slider_icons_size->value());
}

void GameCollectionActivity::load()
{
    QSettings settings;
    QString dirpath = settings.value(SettingsKey::ul_dir).toString();
    QString choosen_dirpath = QFileDialog::getExistingDirectory(this, tr("Choose the OPL root directory"), dirpath);
    if(choosen_dirpath.isEmpty()) return;
    if(choosen_dirpath != dirpath)
        settings.setValue(SettingsKey::ul_dir, choosen_dirpath);
    load(choosen_dirpath);
}

bool GameCollectionActivity::tryLoadRecentDirectory()
{
    QSettings settings;
    QVariant value = settings.value(SettingsKey::ul_dir);
    if(!value.isValid()) return false;
    QDir dir(value.toString());
    if(!dir.exists()) return false;
    load(dir);
    return true;
}

void GameCollectionActivity::load(const QDir & _directory)
{
    Core::GameCollection & game_collection = Application::instance().gameCollection();
    game_collection.load(_directory);
    delete mp_game_art_manager;
    mp_game_art_manager = new Core::GameArtManager(_directory, this);
    connect(mp_game_art_manager, &Core::GameArtManager::artChanged, this, &GameCollectionActivity::gameArtChanged);
    mp_game_art_manager->addCacheType(Core::GameArtType::Icon);
    mp_game_art_manager->addCacheType(Core::GameArtType::Front);
    mp_model->setArtManager(*mp_game_art_manager);
    mp_proxy_model->sort(0, Qt::AscendingOrder);
    if(game_collection.count() > 0)
        mp_tree_games->setCurrentIndex(mp_proxy_model->index(0, 0));
}

void GameCollectionActivity::reload()
{
    load(Application::instance().gameCollection().directory());
}

void GameCollectionActivity::collectionLoaded()
{
    mp_label_directory->setText(Application::instance().gameCollection().directory());
    activateCollectionControls(true);
    gameSelected();
}

void GameCollectionActivity::gameRenamed(const QString & _id)
{
    const Core::Game * game = mp_model->game(mp_proxy_model->mapToSource(mp_tree_games->currentIndex()));
    if(game && game->id() == _id)
        gameSelected();
}

void GameCollectionActivity::gameArtChanged(const QString & _game_id, Core::GameArtType _type, const QPixmap * _pixmap)
{
    if(_type != Core::GameArtType::Front)
        return;
    const Core::Game * game = mp_model->game(mp_proxy_model->mapToSource(mp_tree_games->currentIndex()));
    if(game && game->id() == _game_id)
        mp_label_cover->setPixmap(_pixmap ? *_pixmap : m_default_cover);
}

void GameCollectionActivity::changeIconsSize()
{
    int size = mp_slider_icons_size->value() * 16;
    mp_tree_games->setIconSize(QSize(size, size));
}

void GameCollectionActivity::gameSelected()
{
    const Core::Game * game = mp_model->game(mp_proxy_model->mapToSource(mp_tree_games->currentIndex()));
    if(game)
    {
        mp_label_id->setText(game->id());
        mp_label_title->setText(game->title());
        QPixmap pixmap = mp_game_art_manager->load(game->id(), Core::GameArtType::Front);
        mp_label_cover->setPixmap(pixmap.isNull() ? m_default_cover : pixmap);
        mp_label_type->setText(game->mediaType() == Core::MediaType::CD ? "CD" : "DVD");
        mp_label_parts->setText(QString("%1").arg(game->partCount()));
        mp_label_source->setText(
            game->installationType() == Core::GameInstallationType::UlConfig ? "UL" : tr("Directory"));
        mp_widget_details->show();
    }
    else
    {
        mp_widget_details->hide();
    }
    activateItemControls(game);
}

void GameCollectionActivity::showGameDetails()
{
    const Core::Game * game = mp_model->game(mp_proxy_model->mapToSource(mp_tree_games->currentIndex()));
    if(game)
    {
        QSharedPointer<Intent> intent = GameDetailsActivity::createIntent(*mp_game_art_manager, game->id());
        Application::instance().pushActivity(*intent);
    }
}

void GameCollectionActivity::showIsoRestorer()
{
    const Core::Game * game = mp_model->game(mp_proxy_model->mapToSource(mp_tree_games->currentIndex()));
    if(game && game->installationType() == Core::GameInstallationType::UlConfig)
    {
        QSharedPointer<Intent> intent = IsoRestorerActivity::createIntent(game->id());
        Application::instance().pushActivity(*intent);
    }
}

void GameCollectionActivity::showGameInstaller()
{
    QSharedPointer<Intent> intent = GameInstallerActivity::createIntent();
    Application::instance().pushActivity(*intent);
}
