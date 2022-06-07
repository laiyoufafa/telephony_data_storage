/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "opkey_ability.h"

#include "abs_shared_result_set.h"
#include "data_ability_predicates.h"
#include "values_bucket.h"
#include "predicates_utils.h"

#include "data_storage_errors.h"

namespace OHOS {
using AppExecFwk::AbilityLoader;
using AppExecFwk::Ability;
namespace Telephony {
void OpKeyAbility::OnStart(const AppExecFwk::Want &want)
{
    DATA_STORAGE_LOGI("OpKeyAbility::OnStart\n");
    Ability::OnStart(want);
    std::string path = GetAbilityContext()->GetDatabaseDir();
    DATA_STORAGE_LOGI("GetDatabaseDir: %{public}s", path.c_str());
    if (!path.empty()) {
        initDatabaseDir = true;
        path.append("/");
        helper_.UpdateDbPath(path);
        InitUriMap();
        int rdbInitCode = helper_.Init();
        if (rdbInitCode == NativeRdb::E_OK) {
            initRdbStore = true;
        } else {
            DATA_STORAGE_LOGE("OpKeyAbility::OnStart rdb init failed!");
            initRdbStore = false;
        }
    } else {
        initDatabaseDir = false;
        DATA_STORAGE_LOGE("OpKeyAbility::OnStart##the databaseDir is empty!");
    }
}

int OpKeyAbility::Insert(const Uri &uri, const NativeRdb::ValuesBucket &value)
{
    if (!IsInitOk()) {
        return DATA_STORAGE_ERROR;
    }
    std::lock_guard<std::mutex> guard(lock_);
    Uri tempUri = uri;
    OpKeyUriType opKeyUriType = ParseUriType(tempUri);
    int64_t id = DATA_STORAGE_ERROR;
    if (opKeyUriType == OpKeyUriType::OPKEY_INFO) {
        helper_.Insert(id, value, TABLE_OPKEY_INFO);
    } else {
        DATA_STORAGE_LOGE("OpKeyAbility::Insert failed##uri = %{public}s", uri.ToString().c_str());
    }
    return id;
}

std::shared_ptr<NativeRdb::AbsSharedResultSet> OpKeyAbility::Query(
    const Uri &uri, const std::vector<std::string> &columns, const NativeRdb::DataAbilityPredicates &predicates)
{
    std::shared_ptr<NativeRdb::AbsSharedResultSet> resultSet = nullptr;
    if (!IsInitOk()) {
        return resultSet;
    }
    Uri tempUri = uri;
    OpKeyUriType opKeyUriType = ParseUriType(tempUri);
    if (opKeyUriType == OpKeyUriType::OPKEY_INFO) {
        NativeRdb::AbsRdbPredicates *absRdbPredicates = new NativeRdb::AbsRdbPredicates(TABLE_OPKEY_INFO);
        if (absRdbPredicates != nullptr) {
            ConvertPredicates(predicates, absRdbPredicates);
            resultSet = helper_.Query(*absRdbPredicates, columns);
            delete absRdbPredicates;
            absRdbPredicates = nullptr;
        } else {
            DATA_STORAGE_LOGE("OpKeyAbility::Delete  NativeRdb::AbsRdbPredicates is null!");
        }
    } else {
        DATA_STORAGE_LOGE("OpKeyAbility::Query failed##uri = %{public}s", uri.ToString().c_str());
    }
    return resultSet;
}

int OpKeyAbility::Update(
    const Uri &uri, const NativeRdb::ValuesBucket &value, const NativeRdb::DataAbilityPredicates &predicates)
{
    int result = DATA_STORAGE_ERROR;
    if (!IsInitOk()) {
        return result;
    }
    std::lock_guard<std::mutex> guard(lock_);
    Uri tempUri = uri;
    OpKeyUriType opKeyUriType = ParseUriType(tempUri);
    NativeRdb::AbsRdbPredicates *absRdbPredicates = nullptr;
    switch (opKeyUriType) {
        case OpKeyUriType::OPKEY_INFO: {
            absRdbPredicates = new NativeRdb::AbsRdbPredicates(TABLE_OPKEY_INFO);
            break;
        }
        default:
            DATA_STORAGE_LOGE("OpKeyAbility::Update failed##uri = %{public}s", uri.ToString().c_str());
            break;
    }
    if (absRdbPredicates != nullptr) {
        int changedRows = CHANGED_ROWS;
        ConvertPredicates(predicates, absRdbPredicates);
        result = helper_.Update(changedRows, value, *absRdbPredicates);
        delete absRdbPredicates;
        absRdbPredicates = nullptr;
    } else if (result == DATA_STORAGE_ERROR) {
        DATA_STORAGE_LOGE("OpKeyAbility::Update  NativeRdb::AbsRdbPredicates is null!");
    }
    return result;
}

int OpKeyAbility::Delete(const Uri &uri, const NativeRdb::DataAbilityPredicates &predicates)
{
    int result = DATA_STORAGE_ERROR;
    if (!IsInitOk()) {
        return result;
    }
    std::lock_guard<std::mutex> guard(lock_);
    Uri tempUri = uri;
    OpKeyUriType opKeyUriType = ParseUriType(tempUri);
    if (opKeyUriType == OpKeyUriType::OPKEY_INFO) {
        NativeRdb::AbsRdbPredicates *absRdbPredicates = new NativeRdb::AbsRdbPredicates(TABLE_OPKEY_INFO);
        if (absRdbPredicates != nullptr) {
            ConvertPredicates(predicates, absRdbPredicates);
            int deletedRows = CHANGED_ROWS;
            result = helper_.Delete(deletedRows, *absRdbPredicates);
            delete absRdbPredicates;
            absRdbPredicates = nullptr;
        } else {
            DATA_STORAGE_LOGE("OpKeyAbility::Delete  NativeRdb::AbsRdbPredicates is null!");
        }
    } else {
        DATA_STORAGE_LOGE("OpKeyAbility::Delete failed##uri = %{public}s\n", uri.ToString().c_str());
    }
    return result;
}

bool OpKeyAbility::IsInitOk()
{
    if (!initDatabaseDir) {
        DATA_STORAGE_LOGE("OpKeyAbility::IsInitOk initDatabaseDir failed!");
        return false;
    }
    if (!initRdbStore) {
        DATA_STORAGE_LOGE("OpKeyAbility::IsInitOk initRdbStore failed!");
        return false;
    }
    return true;
}

void OpKeyAbility::InitUriMap()
{
    opKeyUriMap = {
        {"/opkey/opkey_info", OpKeyUriType::OPKEY_INFO}
    };
}

std::string OpKeyAbility::GetType(const Uri &uri)
{
    DATA_STORAGE_LOGI("OpKeyAbility::GetType##uri = %{public}s\n", uri.ToString().c_str());
    std::string retval(uri.ToString());
    return retval;
}

int OpKeyAbility::OpenFile(const Uri &uri, const std::string &mode)
{
    DATA_STORAGE_LOGI("OpKeyAbility::OpenFile##uri = %{public}s\n", uri.ToString().c_str());
    Uri tempUri = uri;
    OpKeyUriType opKeyUriType = ParseUriType(tempUri);
    return static_cast<int>(opKeyUriType);
}

OpKeyUriType OpKeyAbility::ParseUriType(Uri &uri)
{
    DATA_STORAGE_LOGI("OpKeyAbility::ParseUriType start\n");
    OpKeyUriType opKeyUriType = OpKeyUriType::UNKNOW;
    std::string uriPath = uri.ToString();
    if (!uriPath.empty()) {
        helper_.ReplaceAllStr(uriPath, ":///", "://");
        Uri tempUri(uriPath);
        std::string path = tempUri.GetPath();
        if (!path.empty()) {
            DATA_STORAGE_LOGI("OpKeyAbility::ParseUriType##path = %{public}s\n", path.c_str());
            std::map<std::string, OpKeyUriType>::iterator it = opKeyUriMap.find(path);
            if (it != opKeyUriMap.end()) {
                opKeyUriType = it->second;
                DATA_STORAGE_LOGI("OpKeyAbility::ParseUriType##opKeyUriType = %{public}d\n",
                    opKeyUriType);
            }
        }
    }
    return opKeyUriType;
}

void OpKeyAbility::ConvertPredicates(
    const NativeRdb::DataAbilityPredicates &dataPredicates, NativeRdb::AbsRdbPredicates *absRdbPredicates)
{
    NativeRdb::PredicatesUtils::SetWhereClauseAndArgs(
        absRdbPredicates, dataPredicates.GetWhereClause(), dataPredicates.GetWhereArgs());
    NativeRdb::PredicatesUtils::SetAttributes(absRdbPredicates, dataPredicates.IsDistinct(),
        dataPredicates.GetIndex(), dataPredicates.GetGroup(), dataPredicates.GetOrder(), dataPredicates.GetLimit(),
        dataPredicates.GetOffset());
}

REGISTER_AA(OpKeyAbility);
} // namespace Telephony
} // namespace OHOS