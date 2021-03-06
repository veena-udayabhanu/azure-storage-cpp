// -----------------------------------------------------------------------------------------
// <copyright file="service_properties_test.cpp" company="Microsoft">
//    Copyright 2013 Microsoft Corporation
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//      http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
// </copyright>
// -----------------------------------------------------------------------------------------

#include "stdafx.h"
#include "test_base.h"
#include "check_macros.h"

#include "was/blob.h"
#include "was/queue.h"
#include "was/table.h"

void add_metrics_1(wa::storage::service_properties::metrics_properties& metrics)
{
    metrics = wa::storage::service_properties::metrics_properties();
    metrics.set_version(U("1.0"));
    metrics.set_enabled(true);
    metrics.set_include_apis(false);
    metrics.set_retention_policy_enabled(true);
    metrics.set_retention_days(5);
}

void add_metrics_2(wa::storage::service_properties::metrics_properties& metrics)
{
    metrics = wa::storage::service_properties::metrics_properties();
    metrics.set_version(U("1.0"));
    metrics.set_enabled(true);
    metrics.set_include_apis(true);
    metrics.set_retention_policy_enabled(false);
}

void add_logging_1(wa::storage::service_properties::logging_properties& logging)
{
    logging = wa::storage::service_properties::logging_properties();
    logging.set_version(U("1.0"));
    logging.set_read_enabled(true);
    logging.set_retention_policy_enabled(true);
    logging.set_retention_days(20);
}

void add_logging_2(wa::storage::service_properties::logging_properties& logging)
{
    logging = wa::storage::service_properties::logging_properties();
    logging.set_version(U("1.0"));
    logging.set_write_enabled(true);
    logging.set_delete_enabled(true);
    logging.set_retention_policy_enabled(false);
}

void add_cors_rule_1(std::vector<wa::storage::service_properties::cors_rule>& cors_rules)
{
    wa::storage::service_properties::cors_rule rule;
    rule.allowed_headers().push_back(U("x-ms-meta-data*"));
    rule.allowed_headers().push_back(U("x-ms-meta-target*"));
    rule.allowed_origins().push_back(U("www.ab.com"));
    rule.allowed_origins().push_back(U("www.bc.com"));
    rule.allowed_methods().push_back(web::http::methods::GET);
    rule.allowed_methods().push_back(web::http::methods::PUT);
    rule.exposed_headers().push_back(U("x-ms-meta-source*"));
    rule.exposed_headers().push_back(U("x-ms-meta-test*"));
    rule.set_max_age(std::chrono::seconds(5));
    cors_rules.push_back(rule);
}

void add_cors_rule_2(std::vector<wa::storage::service_properties::cors_rule>& cors_rules)
{
    wa::storage::service_properties::cors_rule rule;
    rule.allowed_headers().push_back(U("x-ms-meta-ab*"));
    rule.allowed_origins().push_back(U("*"));
    rule.allowed_methods().push_back(web::http::methods::HEAD);
    rule.exposed_headers().push_back(U("x-ms-meta-abc*"));
    rule.set_max_age(std::chrono::seconds(25));
    cors_rules.push_back(rule);
}

void check_service_properties(const wa::storage::service_properties& a, const wa::storage::service_properties& b)
{
    CHECK_UTF8_EQUAL(a.logging().version(), b.logging().version());
    CHECK_EQUAL(a.logging().delete_enabled(), b.logging().delete_enabled());
    CHECK_EQUAL(a.logging().read_enabled(), b.logging().read_enabled());
    CHECK_EQUAL(a.logging().write_enabled(), b.logging().write_enabled());
    CHECK_EQUAL(a.logging().retention_policy_enabled(), b.logging().retention_policy_enabled());
    CHECK_EQUAL(a.logging().retention_days(), b.logging().retention_days());

    CHECK_UTF8_EQUAL(a.hour_metrics().version(), b.hour_metrics().version());
    CHECK_EQUAL(a.hour_metrics().enabled(), b.hour_metrics().enabled());
    CHECK_EQUAL(a.hour_metrics().include_apis(), b.hour_metrics().include_apis());
    CHECK_EQUAL(a.hour_metrics().retention_policy_enabled(), b.hour_metrics().retention_policy_enabled());
    CHECK_EQUAL(a.hour_metrics().retention_days(), b.hour_metrics().retention_days());

    CHECK_UTF8_EQUAL(a.minute_metrics().version(), b.minute_metrics().version());
    CHECK_EQUAL(a.minute_metrics().enabled(), b.minute_metrics().enabled());
    CHECK_EQUAL(a.minute_metrics().include_apis(), b.minute_metrics().include_apis());
    CHECK_EQUAL(a.minute_metrics().retention_policy_enabled(), b.minute_metrics().retention_policy_enabled());
    CHECK_EQUAL(a.minute_metrics().retention_days(), b.minute_metrics().retention_days());

    auto a_iter = a.cors().cbegin();
    auto b_iter = b.cors().cbegin();
    for (; a_iter != a.cors().cend() && b_iter != b.cors().cend(); ++a_iter, ++b_iter)
    {
        CHECK(std::equal(a_iter->allowed_headers().cbegin(), a_iter->allowed_headers().cend(), b_iter->allowed_headers().cbegin()));
        CHECK(std::equal(a_iter->allowed_methods().cbegin(), a_iter->allowed_methods().cend(), b_iter->allowed_methods().cbegin()));
        CHECK(std::equal(a_iter->allowed_origins().cbegin(), a_iter->allowed_origins().cend(), b_iter->allowed_origins().cbegin()));
        CHECK(std::equal(a_iter->exposed_headers().cbegin(), a_iter->exposed_headers().cend(), b_iter->exposed_headers().cbegin()));
        CHECK_EQUAL(a_iter->max_age().count(), b_iter->max_age().count());
    }

    CHECK(a_iter == a.cors().cend());
    CHECK(b_iter == b.cors().cend());

    CHECK_UTF8_EQUAL(a.default_service_version(), b.default_service_version());
}

template<typename Client, typename Options>
void test_service_properties(const Client& client, const Options& options, wa::storage::operation_context context, bool default_version_supported)
{
    wa::storage::service_properties props;
    add_logging_1(props.logging());
    add_metrics_1(props.hour_metrics());
    add_metrics_2(props.minute_metrics());
    add_cors_rule_1(props.cors());
    add_cors_rule_2(props.cors());

    wa::storage::service_properties temp_props;
    add_logging_2(temp_props.logging());
    add_metrics_2(temp_props.hour_metrics());
    add_metrics_1(temp_props.minute_metrics());
    add_cors_rule_2(temp_props.cors());

    client.upload_service_properties(props, wa::storage::service_properties_includes::all(), options, context);
    check_service_properties(props, client.download_service_properties(options, context));

    {
        wa::storage::service_properties_includes includes;
        includes.set_logging(true);
        client.upload_service_properties(temp_props, includes, options, context);
        add_logging_2(props.logging());
        check_service_properties(props, client.download_service_properties(options, context));
        add_logging_1(temp_props.logging());
    }

    {
        wa::storage::service_properties_includes includes;
        includes.set_hour_metrics(true);
        client.upload_service_properties(temp_props, includes, options, context);
        add_metrics_2(props.hour_metrics());
        check_service_properties(props, client.download_service_properties(options, context));
        add_metrics_1(temp_props.hour_metrics());
    }

    {
        wa::storage::service_properties_includes includes;
        includes.set_minute_metrics(true);
        client.upload_service_properties(temp_props, includes, options, context);
        add_metrics_1(props.minute_metrics());
        check_service_properties(props, client.download_service_properties(options, context));
        add_metrics_2(temp_props.minute_metrics());
    }

    {
        wa::storage::service_properties_includes includes;
        includes.set_cors(true);
        client.upload_service_properties(temp_props, includes, options, context);
        props.cors().erase(props.cors().begin());
        check_service_properties(props, client.download_service_properties(options, context));
        temp_props.cors().clear();
    }

    {
        wa::storage::service_properties_includes includes;
        includes.set_hour_metrics(true);
        includes.set_minute_metrics(true);
        includes.set_cors(true);
        client.upload_service_properties(temp_props, includes, options, context);
        add_metrics_1(props.hour_metrics());
        add_metrics_2(props.minute_metrics());
        props.cors().clear();
        check_service_properties(props, client.download_service_properties(options, context));
    }

    props.set_default_service_version(U("2013-08-15"));
    if (default_version_supported)
    {
        client.upload_service_properties(props, wa::storage::service_properties_includes::all(), options, context);
        check_service_properties(props, client.download_service_properties(options, context));
    }
    else
    {
        CHECK_THROW(client.upload_service_properties(props, wa::storage::service_properties_includes::all(), options, context), wa::storage::storage_exception);
    }
}

SUITE(Client)
{
    TEST_FIXTURE(test_base, blob_service_properties)
    {
        auto client = test_config::instance().account().create_cloud_blob_client();
        test_service_properties(client, wa::storage::blob_request_options(), m_context, true);
    }

    TEST_FIXTURE(test_base, queue_service_properties)
    {
        auto client = test_config::instance().account().create_cloud_queue_client();
        test_service_properties(client, wa::storage::queue_request_options(), m_context, false);
    }

    TEST_FIXTURE(test_base, table_service_properties)
    {
        auto client = test_config::instance().account().create_cloud_table_client();
        test_service_properties(client, wa::storage::table_request_options(), m_context, false);
    }
}
