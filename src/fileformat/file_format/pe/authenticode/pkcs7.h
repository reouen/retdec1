/**
 * @file src/fileformat/file_format/pe/authenticode/pkcs7.h
 * @brief Class wrapper above openssl Pkcs7
 * @copyright (c) 2020 Avast Software, licensed under the MIT license
 */

#pragma once

#include "authenticode_structs.h"
#include "helper.h"
#include "x509_certificate.h"
#include "pkcs9.h"
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

class Pkcs7
{
	struct SpcSpOpusInfo
	{
		SpcSpOpusInfo(const unsigned char* data, int len) noexcept;
	};
	class ContentInfo
	{
	private:
	public:
		std::string contentType;
		std::string digest;
		Algorithms digestAlgorithm;

		ContentInfo() = default;
		ContentInfo(const PKCS7* pkcs7);
	};
	class SignerInfo
	{
	private:
		void parseUnauthAttrs(PKCS7_SIGNER_INFO* si_info, STACK_OF(X509)* raw_certs);
		void parseAuthAttrs(PKCS7_SIGNER_INFO* si_info);

		std::unique_ptr<STACK_OF(X509), decltype(&sk_X509_free)> raw_signers;
		const X509* signerCert = nullptr;
		// SpcSpOpusInfo spcInfo; /* TODO decide what to do with this information as it's only optional */
	public:
		std::uint64_t version;

		std::string serial;
		std::string issuer;
		std::string contentType; /* TODO decide if we should store oid or name repre? */
		std::string messageDigest;

		Algorithms digestAlgorithm; /* Must be identical to SignedData::digestAlgorithm */
		Algorithms digestEncryptAlgorithm;

		std::vector<std::uint8_t> encryptDigest;
		std::vector<Pkcs7> nestedSignatures;
		std::vector<Pkcs9> counterSignatures;
		std::vector<MsCounterSignature> msSignatures;
		/* TODO add ms counter signatures */

		const X509* getSignerCert() const;

		auto operator=(const SignerInfo&) = delete;
		SignerInfo(const SignerInfo&) = delete;

		SignerInfo& operator=(SignerInfo&&) noexcept = default;
		SignerInfo(SignerInfo&&) noexcept = default;

		SignerInfo(PKCS7* pkcs7, STACK_OF(X509)* raw_certs);
	};

private:
	std::unique_ptr<PKCS7, decltype(&PKCS7_free)> pkcs7;

	STACK_OF(X509)* getCertificates() const;
	STACK_OF(X509)* getSigners();

	void parseSignerInfo(PKCS7_SIGNER_INFO* si_info);
	void parseCertificates(PKCS7_SIGNER_INFO* info);

public:
	std::uint64_t version;
	ContentInfo contentInfo;
	std::optional<SignerInfo> signerInfo;

	Algorithms contentDigestAlgorithm; // must match SignerInfo digestAlgorithm
	std::vector<X509Certificate> certificates; /* typically no root certificates, timestamp may include root one */

	std::vector<retdec::fileformat::DigitalSignature> getSignatures() const;
	std::vector<std::string> warnings;

	Pkcs7& operator=(const Pkcs7&) = delete;
	Pkcs7(const Pkcs7&) = delete;

	Pkcs7& operator=(Pkcs7&&) noexcept = default;
	Pkcs7(Pkcs7&&) noexcept = default;

	Pkcs7(const std::vector<std::uint8_t>& input) noexcept;
};

} // namespace authenticode