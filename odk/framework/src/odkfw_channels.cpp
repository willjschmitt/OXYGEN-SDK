// Copyright DEWETRON GmbH 2019
#include "odkfw_channels.h"
#include "odkfw_properties.h"
#include "odkapi_utils.h"

//TODO - other telegram
#include "odkapi_channel_config_changed_xml.h"
#include "odkapi_channel_dataformat_xml.h"
#include "odkapi_channel_mapping_xml.h"

namespace
{
    template <class T>
    typename T::value_type get_optional_value(const T& container, const typename T::key_type& key)
    {
        auto it = container.find(key);
        if (it != container.cend())
        {
            return it->second;
        }
        else
        {
            return {};
        }
    }

}

namespace odk
{
namespace framework
{

    PluginChannel::PluginChannel(std::uint32_t local_id, IfPluginChannelChangeListener* change_listener)
        : m_change_listener(change_listener)
    {
        m_channel_info.m_local_id = local_id;
    }

    PluginChannel::~PluginChannel()
    {
        for (auto p : m_properties)
        {
            p.second->setChangeListener(nullptr);
        }
    }

    std::uint32_t PluginChannel::getLocalId() const
    {
        return m_channel_info.m_local_id;
    }

    PluginChannel& PluginChannel::setSampleFormat(
        odk::ChannelDataformat::SampleOccurrence occurrence, odk::ChannelDataformat::SampleFormat format, std::uint32_t dimension)
    {
        m_channel_info.setSampleFormat(occurrence, format, dimension);
        if (m_change_listener) m_change_listener->onChannelSetupChanged(this);
        return *this;
    }

    PluginChannel& PluginChannel::setSimpleTimebase(double frequency)
    {
        m_channel_info.setSimpleTimebase(frequency);
        if (m_change_listener) m_change_listener->onChannelSetupChanged(this);
        return *this;
    }

    PluginChannel& PluginChannel::setDefaultName(const std::string& name)
    {
        m_channel_info.setDefaultName(name);
        if (m_change_listener) m_change_listener->onChannelSetupChanged(this);
        return *this;
    }
    std::string PluginChannel::getDefaultName() const
    {
        return m_channel_info.m_default_name;
    }

    PluginChannel& PluginChannel::setDomain(const std::string& name)
    {
        m_channel_info.setDomain(name);
        if (m_change_listener) m_change_listener->onChannelSetupChanged(this);
        return *this;
    }
    std::string PluginChannel::getDomain() const
    {
        return m_channel_info.m_domain;
    }

    PluginChannel& PluginChannel::setValid(const bool valid)
    {
        m_channel_info.setValid(valid);
        if (m_change_listener) m_change_listener->onChannelSetupChanged(this);
        return *this;
    }

    PluginChannel &PluginChannel::setRange(const odk::Range& range)
    {
        auto range_property = getRangeProperty();
        if(range_property)
        {
            range_property->setValue(range);
        }
        else
        {
            addProperty("Range", std::make_shared<RangeProperty>(range));
        }
        return *this;
    }

    PluginChannel &PluginChannel::setUnit(const std::string& unit)
    {
        auto unit_property = getUnitProperty();
        if(unit_property)
        {
            unit_property->setValue(unit);
        }
        else
        {
            addProperty("Unit", std::make_shared<EditableStringProperty>(unit));
        }
        return *this;
    }

    PluginChannel& PluginChannel::setDeletable(bool deletable)
    {
        m_channel_info.setDeletable(deletable);
        if (m_change_listener) m_change_listener->onChannelSetupChanged(this);
        return *this;
    }

    PluginChannelPtr PluginChannel::getLocalParent() const
    {
        return m_local_parent;
    }

    PluginChannel& PluginChannel::setLocalParent(PluginChannelPtr ch)
    {
        m_local_parent = ch;
        m_channel_info.setLocalParent(ch ? ch->getLocalId() : -1);
        if (m_change_listener) m_change_listener->onChannelSetupChanged(this);
        return *this;
    }

    PluginChannel& PluginChannel::addProperty(const std::string& name, ChannelPropertyPtr prop)
    {
        prop->setChangeListener(this);
        m_properties.push_back(std::make_pair(name, prop));
        if (m_change_listener) m_change_listener->onChannelPropertyChanged(this, name);
        return *this;
    }

    ChannelPropertyPtr PluginChannel::getProperty(const std::string& name) const
    {
        for (auto& prop : m_properties)
        {
            if (prop.first == name)
            {
                return prop.second;
            }
        }
        return {};
    }

    std::shared_ptr<BooleanProperty> PluginChannel::getUsedProperty() const
    {
        return std::dynamic_pointer_cast<BooleanProperty>(getProperty("Used"));
    }

    std::shared_ptr<RangeProperty> PluginChannel::getRangeProperty() const
    {
        return std::dynamic_pointer_cast<RangeProperty>(getProperty("Range"));
    }

    std::shared_ptr<EditableStringProperty> PluginChannel::getUnitProperty() const
    {
        return std::dynamic_pointer_cast<EditableStringProperty>(getProperty("Unit"));
    }

    const std::vector<std::pair<std::string, ChannelPropertyPtr> > &PluginChannel::getProperties()
    {
        return m_properties;
    }

    void PluginChannel::setChangeListener(IfPluginChannelChangeListener* l)
    {
        m_change_listener = l;
    }

    void PluginChannel::onChannelPropertyChanged(const IfChannelProperty* channel)
    {
        std::string name;
        for (const auto p : m_properties)
        {
            if (p.second.get() == channel)
            {
                name = p.first;
                break;
            }
        }
        if (m_change_listener) m_change_listener->onChannelPropertyChanged(this, name);
    }

    PluginTask::PluginTask(std::uint64_t id, IfPluginTaskChangeListener* l, std::shared_ptr<IfTaskWorker> worker, std::uint64_t token)
        : m_change_listener(l)
        , m_id(id)
        , m_registered(false)
        , m_worker(worker)
        , m_token(token)
        , m_valid(false)
    {
    }

    void PluginTask::clearAllInputChannels()
    {
        m_input_channels.clear();
        if (m_change_listener) m_change_listener->onTaskInputChannelsChanged(this);
    }

    void PluginTask::addInputChannel(std::uint64_t ch_id)
    {
        ODK_ASSERT(std::find(m_input_channels.begin(), m_input_channels.end(), ch_id) == m_input_channels.end());
        if (m_change_listener) m_change_listener->onTaskInputChannelsChanged(this);
        m_input_channels.push_back(ch_id);
    }

    void PluginTask::addOutputChannel(PluginChannelPtr ch)
    {
        ODK_ASSERT(std::find(m_output_channels.begin(), m_output_channels.end(), ch) == m_output_channels.end());
        if (m_change_listener) m_change_listener->onTaskChannelAdded(this, ch.get());
        m_output_channels.push_back(ch);
    }

    void PluginTask::removeOutputChannel(PluginChannelPtr ch)
    {
        auto it = std::find(m_output_channels.begin(), m_output_channels.end(), ch);
        ODK_ASSERT(it != m_output_channels.end());
        if (it != m_output_channels.end())
        {
            if (m_change_listener) m_change_listener->onTaskChannelRemoved(this, ch.get());
            m_output_channels.erase(it);
        }
    }

    uint64_t PluginTask::getID()
    {
        return m_id;
    }

    bool PluginTask::isValid() const
    {
        return m_valid;
    }

    void PluginTask::setValid(bool valid)
    {
        m_valid = valid;
    }


    void PluginTask::setChangeListener(IfPluginTaskChangeListener* l)
    {
        m_change_listener = l;
    }

    PluginChannels::~PluginChannels()
    {
        for (auto p : m_channels)
        {
            p.second->setChangeListener(nullptr);
        }
    }

    PluginChannelPtr PluginChannels::addChannel()
    {
        auto local_id = generateId();
        auto ret = std::make_shared<PluginChannel>(local_id, this);
        m_channels[local_id] = ret;
        m_channels_dirty = true;
        return ret;
    }

    void PluginChannels::removeChannel(PluginChannelPtr ch)
    {
        ch->setChangeListener(nullptr);
        m_channels.erase(ch->m_channel_info.m_local_id);
        m_channels_dirty = true;

        for (auto& c : m_channels)
        {
            if (c.second->getLocalParent() == ch)
            {
                c.second->setLocalParent({});
            }
        }

        auto task_map_it = m_channel_to_task.find(ch->m_channel_info.m_local_id);
        if (task_map_it != m_channel_to_task.end())
        {
            auto task = m_tasks[task_map_it->second];
            ODK_ASSERT(task);
            m_channel_to_task.erase(task_map_it);
            task->removeOutputChannel(ch);
        }
    }

    void PluginChannels::setPluginHost(odk::IfHost* host)
    {
        m_host = host;
    }

    std::shared_ptr<PluginTask> PluginChannels::addTask(IfTaskWorker* worker, uint64_t token)
    {
        auto no_delete = [](IfTaskWorker* worker) {};
        auto wrapped_worker = std::shared_ptr<IfTaskWorker>(worker, no_delete);
        return addTask(wrapped_worker, token);
    }

    std::shared_ptr<PluginTask> PluginChannels::addTask(std::shared_ptr<IfTaskWorker> worker, uint64_t token)
    {
        auto task = std::make_shared<PluginTask>(m_next_task_id++, this, worker, token);
        m_tasks[task->m_id] = task;
        return task;
    }

    void PluginChannels::removeTask(std::shared_ptr<PluginTask> task)
    {
        unregisterTask(*task);

        for (auto it = m_channel_to_task.cbegin(); it != m_channel_to_task.cend();)
        {
            if (it->second == task->getID())
            {
                it = m_channel_to_task.erase(it);
            }
            else
            {
                ++it;
            }
        }
        m_tasks.erase(task->m_id);
    }

    void PluginChannels::synchronize()
    {
        ODK_ASSERT(m_host);
        if (m_channels_dirty)
        {
            odk::UpdateChannelsTelegram telegram;
            for (const auto& channel : m_channels)
            {
                ODK_ASSERT(channel.second->m_channel_info.m_local_parent_id == (channel.second->m_local_parent ? channel.second->m_local_parent->getLocalId() : -1));
                telegram.appendChannel(channel.second->m_channel_info);
            }
            telegram.m_list_topology = m_list_topology;
            {
                auto xml = telegram.generate();
                m_host->messageSyncData(odk::host_msg::SET_PLUGIN_OUTPUT_CHANNELS, 0, xml.c_str(), xml.size() + 1, nullptr);
            }
        }
        if (!m_properties_dirty.empty())
        {
            odk::UpdateConfigTelegram telegram;
            for (const auto& channel : m_properties_dirty)
            {
                if (m_channels.find(channel->getLocalId()) != m_channels.end())
                {
                    auto& tg_ch = telegram.addChannel(channel->m_channel_info.m_local_id);
                    for (const auto& prop : channel->m_properties)
                    {
                        prop.second->addToTelegram(tg_ch, prop.first);
                    }
                }
            }

            if (!telegram.m_channel_configs.empty())
            {
                auto xml = telegram.generate();
                m_host->messageSyncData(odk::host_msg::SET_PLUGIN_CONFIGURATION, 0, xml.c_str(), xml.size() + 1, nullptr);
            }
        }

        //only reuse ids after sync to avoid transparently replacing a channel
        resetUsedIds();
        m_channels_dirty = false;
        m_properties_dirty.clear();

        for (auto& task : m_tasks)
        {
            registerTask(*task.second);
        }
    }

    void PluginChannels::reset()
    {
        pauseTasks();
        m_tasks.clear();

        m_channels.clear();
        m_ids.clear();
        m_list_topology = {};

        m_channel_to_task.clear();
        m_next_task_id = 0;
        m_channels_dirty = true;

        synchronize();
    }

    void PluginChannels::pauseTask(PluginTaskPtr& task)
    {
        unregisterTask(*task);
    }

    void PluginChannels::pauseTasks()
    {
        for (auto& task : m_tasks)
        {
            unregisterTask(*task.second);
        }
    }

    std::set<PluginTaskPtr> PluginChannels::getAffectedTasks(const odk::UpdateConfigTelegram& request)
    {
        std::set<PluginTaskPtr> affected_tasks;
        for (const auto& ch_changes : request.m_channel_configs)
        {
            auto task_map_it = m_channel_to_task.find(ch_changes.m_channel_info.m_local_id);
            if (task_map_it != m_channel_to_task.end())
            {
                auto task = m_tasks[task_map_it->second];
                ODK_ASSERT(task);
                affected_tasks.insert(task);
            }
        }
        return affected_tasks;
    }

    std::set<PluginTaskPtr> PluginChannels::getAffectedTasks(std::uint64_t input_channel_id)
    {
        std::set<PluginTaskPtr> affected_tasks;
        for (const auto a_task : m_tasks)
        {
            const auto input_chn_it = std::find(
                a_task.second->m_input_channels.cbegin(),
                a_task.second->m_input_channels.cend(),
                input_channel_id);

            if (input_chn_it != a_task.second->m_input_channels.cend())
            {
                affected_tasks.insert(a_task.second);
            }
        }
        return affected_tasks;
    }

    std::uint64_t PluginChannels::processConfigUpdate(const odk::UpdateConfigTelegram& request)
    {
        odk::UpdateConfigTelegram response;

        const auto affected_tasks = getAffectedTasks(request);

        for (const auto& affected_task : affected_tasks)
        {
            unregisterTask(*affected_task);
        }

        bool unknown_property = false;
        for (const auto& ch_changes : request.m_channel_configs)
        {
            auto it = m_channels.find(ch_changes.m_channel_info.m_local_id);
            if (it != m_channels.end())
            {
                auto ch = it->second;

                for (const auto& ch_change : ch_changes.m_properties)
                {
                    try
                    {
                        auto prop_name = ch_change.getName();
                        auto prop = ch->getProperty(prop_name);
                        if (prop)
                        {
                            auto r = prop->update(ch_change);
                        }
                        else
                        {
                            unknown_property = true;
                        }
                    }
                    catch (...)
                    {
                    }
                }

                auto& tg_ch = response.addChannel(ch->m_channel_info.m_local_id);
                for (const auto& prop : ch->m_properties)
                {
                    prop.second->addToTelegram(tg_ch, prop.first);
                }
            }
        }

        if (!response.m_channel_configs.empty())
        {
            auto xml_msg = m_host->createValue<odk::IfXMLValue>();
            xml_msg->set(response.generate().c_str());
            m_host->messageSync(odk::host_msg::SET_PLUGIN_CONFIGURATION, 0, xml_msg.get(), nullptr);
        }

        for (const auto& affected_task : affected_tasks)
        {
            affected_task->m_worker->onChannelConfigChanged(m_host, affected_task->m_token);
            if (affected_task->isValid())
            {
                registerTask(*affected_task);
            }
        }

        return odk::error_codes::OK;
    }

    uint64_t PluginChannels::processInputChannelChange(const std::set<std::uint64_t>& channel_ids)
    {
        std::set<PluginTaskPtr> affected_tasks;
        for(const auto& channel_id : channel_ids)
        {
            const auto affected_tasks_by_id = getAffectedTasks(channel_id);
            affected_tasks.insert(affected_tasks_by_id.begin(), affected_tasks_by_id.end());
        }

        for (const auto& affected_task : affected_tasks)
        {
            unregisterTask(*affected_task);
        }

        for (const auto& affected_task : affected_tasks)
        {
            affected_task->m_worker->onChannelConfigChanged(m_host, affected_task->m_token);
        }

        for (const auto& affected_task : affected_tasks)
        {
            registerTask(*affected_task);
        }

        return odk::error_codes::OK;
    }

    std::uint64_t PluginChannels::processDataFormatChange(const odk::ChannelDataformatTelegram& request)
    {
        return processInputChannelChange({request.channel_id});
    }

    uint64_t PluginChannels::processInputChannelConfigChange(const odk::ChannelConfigChangedTelegram& telegram)
    {
        std::set<std::uint64_t> channel_ids;
        for(const auto& channel_config : telegram.m_channel_configs)
        {
            channel_ids.insert(channel_config.m_channel_id);
        }
        return processInputChannelChange(channel_ids);
    }

    void PluginChannels::onTaskChannelAdded(PluginTask* task, const PluginChannel* channel)
    {
        unregisterTask(*task);
        m_channel_to_task[channel->getLocalId()] = task->m_id;
    }

    void PluginChannels::onTaskChannelRemoved(PluginTask* task, const PluginChannel* channel)
    {
        unregisterTask(*task);
        m_channel_to_task.erase(channel->getLocalId());
    }

    void PluginChannels::onTaskInputChannelsChanged(PluginTask *task)
    {
        unregisterTask(*task);
    }

    PluginTaskPtr PluginChannels::findTask(std::uint64_t id)
    {
        auto it = m_tasks.find(id);
        if (it != m_tasks.end())
        {
            return it->second;
        }
        else
        {
            return {};
        }
    }

    std::uint32_t PluginChannels::generateId()
    {
        std::uint32_t id = 0;
        {
            while (true)
            {
                if (m_ids.find(id) == m_ids.end())
                {
                    m_ids.insert(id);
                    break;
                }
                else
                {
                    ++id;
                }
            }
        }
        return id;
    }

    void PluginChannels::resetUsedIds()
    {
        m_ids.clear();
        for (const auto& ch : m_channels)
        {
            m_ids.insert(ch.first);
        }
    }

    void PluginChannels::registerTask(PluginTask& task)
    {
        if (!task.m_registered && task.isValid())
        {
            odk::AddAcquisitionTaskTelegram telegram;
            telegram.m_id = task.m_id;

            for (const auto& ch : task.m_input_channels)
            {
                telegram.m_input_channels.push_back(ch);
            }

            for (const auto& ch : task.m_output_channels)
            {
                telegram.m_output_channels.push_back(ch->getLocalId());
            }

            task.m_registered = true;

            auto xml_msg = m_host->createValue<odk::IfXMLValue>();
            xml_msg->set(telegram.generate().c_str());
            m_host->messageSync(odk::host_msg::ACQUISITION_TASK_ADD, 0, xml_msg.get());
        }
    }

    void PluginChannels::unregisterTask(PluginTask& t)
    {
        if (t.m_registered)
        {
            m_host->messageSync(odk::host_msg::ACQUISITION_TASK_REMOVE, t.m_id, nullptr);
            t.m_registered = false;
        }
    }

    void PluginChannels::onChannelPropertyChanged(const PluginChannel* channel, const std::string& name)
    {
        m_properties_dirty.insert(channel);
    }

    void PluginChannels::onChannelSetupChanged(const PluginChannel* channel)
    {
        m_channels_dirty = true;
    }

    std::uint64_t PluginChannels::pluginMessage(odk::PluginMessageId id, std::uint64_t key, const odk::IfValue* param, const odk::IfValue** ret)
    {
        switch (id)
        {
        case odk::plugin_msg::ACQUISITION_TASK_INIT_TIMEBASES:
            if (auto task = findTask(key))
            {
                if (task->m_worker)
                {
                    task->m_worker->onInitTimebases(m_host, task->m_token);
                }
                return odk::error_codes::OK;
            }
            break;
        case odk::plugin_msg::ACQUISITION_TASK_START_PROCESSING:
            if (auto task = findTask(key))
            {
                if (task->m_worker)
                {
                    task->m_worker->onStartProcessing(m_host, task->m_token);
                }
                return odk::error_codes::OK;
            }
            break;
        case odk::plugin_msg::ACQUISITION_TASK_STOP_PROCESSING:
            if (auto task = findTask(key))
            {
                if (task->m_worker)
                {
                    task->m_worker->onStopProcessing(m_host, task->m_token);
                }
                return odk::error_codes::OK;
            }
            break;
        case odk::plugin_msg::ACQUISITION_TASK_PROCESS:
            if (auto task = findTask(key))
            {
                if (task->m_worker)
                {
                    task->m_worker->onProcess(m_host, task->m_token);
                }
                return odk::error_codes::OK;
            }
            break;
        case odk::plugin_msg::PLUGIN_CONFIGURATION_CHANGE_REQUEST:
            if (auto change_xml = odk::value_cast<const odk::IfXMLValue>(param))
            {
                odk::UpdateConfigTelegram update_telegram;
                if (update_telegram.parse(change_xml->getValue()))
                {
                    return processConfigUpdate(update_telegram);
                }
            }
            return odk::error_codes::INVALID_INPUT_PARAMETER;
        case odk::plugin_msg::PLUGIN_CHANNEL_DATAFORMAT_CHANGED:
            {
                odk::ChannelDataformatTelegram telegram;
                if (parseXMLValue(param, telegram))
                {
                    return processDataFormatChange(telegram);
                }
            }
            return odk::error_codes::INVALID_INPUT_PARAMETER;
        case odk::plugin_msg::NOTIFY_CHANNEL_CONFIG_CHANGED:
            {
                odk::ChannelConfigChangedTelegram telegram;

                if (parseXMLValue(param, telegram))
                {
                    return processInputChannelConfigChange(telegram);
                }
                return odk::error_codes::INVALID_INPUT_PARAMETER;
            }
        default:
            return odk::error_codes::NOT_IMPLEMENTED;
        }
        return odk::error_codes::NOT_IMPLEMENTED;
    }

}
}

