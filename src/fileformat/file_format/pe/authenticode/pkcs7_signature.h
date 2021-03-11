/**
 * @file src/fileformat/file_format/pe/authenticode/pkcs7.h
 * @brief Class wrapper above openssl Pkcs7
 * @copyright (c) 2021 Avast Software, licensed under the MIT license
 */

#pragma once

#include "authenticode_structs.h"
#include "helper.h"
#include "retdec/fileformat/types/certificate_table/certificate.h"
#include "retdec/fileformat/types/certificate_table/certificate_table.h"
#include "x509_certificate.h"
#include "pkcs9_counter_signature.h"
#include "ms_counter_signature.h"

#include "retdec/fileformat/types/certificate_table/certificate_table.h"

#include <algorithm>
#include <memory>
#include <openssl/bn.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/ocsp.h>
#include <openssl/pkcs7.h>
#include <openssl/ts.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/pkcs7.h>
#include <openssl/err.h>

#include <array>
#include <vector>
#include <string>
#include <cstdint>
#include <ctime>
#include <optional>
#include <cstdint>
#include <exception>
#include <cstring>

namespace authenticode {

class Pkcs7Signature
{
	struct SpcSpOpusInfo
	{
		SpcSpOpusInfo(const unsigned char* data, int len) noexcept;
	};
	class ContentInfo
	{
	public:
		int contentType;
		std::string digest;
		int digestAlgorithm;

		ContentInfo(const PKCS7* pkcs7);
	};
	class SignerInfo
	{
	private:
		void parseUnauthAttrs(const PKCS7_SIGNER_INFO* si_info, const STACK_OF(X509)* raw_certs);
		void parseAuthAttrs(const PKCS7_SIGNER_INFO* si_info);

		std::unique_ptr<STACK_OF(X509), decltype(&sk_X509_free)> raw_signers;
		const X509* signerCert = nullptr;
		const PKCS7_SIGNER_INFO* sinfo = nullptr;
	public:
		std::uint64_t version;

		std::string serial;
		std::string issuer;
		std::string contentType; /* TODO decide if we should store oid or name repre? */
		std::string messageDigest;
		std::optional<SpcSpOpusInfo> spcInfo;

		int digestAlgorithm; /* Must be identical to SignedData::digestAlgorithm */
		int digestEncryptAlgorithm;

		std::vector<std::uint8_t> encryptDigest;
		std::vector<Pkcs7Signature> nestedSignatures;
		std::vector<Pkcs9CounterSignature> counterSignatures;
		std::vector<MsCounterSignature> msCounterSignatures;

		const X509* getSignerCert() const;
		const PKCS7_SIGNER_INFO* getSignerInfo() const;
		auto operator=(const SignerInfo&) = delete;
		SignerInfo(const SignerInfo&) = delete;

		SignerInfo& operator=(SignerInfo&&) noexcept = default;
		SignerInfo(SignerInfo&&) noexcept = default;

		SignerInfo(const PKCS7* pkcs7, const PKCS7_SIGNER_INFO* si_info, const STACK_OF(X509)* raw_certs);
	};

private:
	std::unique_ptr<PKCS7, decltype(&PKCS7_free)> pkcs7;

public:
	std::uint64_t version;
	std::optional<ContentInfo> contentInfo;
	std::optional<SignerInfo> signerInfo;

	std::vector<int> contentDigestAlgorithms;
	std::vector<X509Certificate> certificates; /* typically no root certificates, timestamp may include root one */

	std::vector<std::string> verify() const;
	std::vector<retdec::fileformat::DigitalSignature> getSignatures() const;
	std::vector<Certificate> getAllCertificates() const;


	Pkcs7Signature& operator=(const Pkcs7Signature&) = delete;
	Pkcs7Signature(const Pkcs7Signature&) = delete;

	Pkcs7Signature& operator=(Pkcs7Signature&&) noexcept = default;
	Pkcs7Signature(Pkcs7Signature&&) noexcept = default;

	Pkcs7Signature(const std::vector<std::uint8_t>& input) noexcept;
};

} // namespace authenticode