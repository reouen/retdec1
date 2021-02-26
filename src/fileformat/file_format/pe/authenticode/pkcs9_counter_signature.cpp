/**
 * @file src/fileformat/file_format/pe/authenticode/pkcs9.cpp
 * @brief Class that wraps openssl pkcs9 information.
 * @copyright (c) 2020 Avast Software, licensed under the MIT license
 */

#include "pkcs9_counter_signature.h"
#include "helper.h"
#include <cstdint>
#include <openssl/pkcs7.h>
#include <openssl/ts.h>
#include <openssl/x509.h>
#include <stdexcept>

namespace authenticode {

/* PKCS7 stores all certificates for the signer and counter signers, we need to pass the certs */
Pkcs9CounterSignature::Pkcs9CounterSignature(std::vector<std::uint8_t>& data, const STACK_OF(X509)* certificates)
{
	/*
		counterSignature ATTRIBUTE ::= {
		  WITH SYNTAX SignerInfo
		  ID pkcs-9-at-counterSignature
		}
	*/
	const unsigned char* data_ptr = data.data();
	PKCS7_SIGNER_INFO* countersignInfo = d2i_PKCS7_SIGNER_INFO(nullptr, &data_ptr, data.size());
	if (!countersignInfo) {
		throw std::runtime_error("SignerInfo allocation failed");
	}

	/* get the signer certificate of this counter signatures */
	signerCert = X509_find_by_issuer_and_serial(const_cast<STACK_OF(X509)*>(certificates),
			countersignInfo->issuer_and_serial->issuer, countersignInfo->issuer_and_serial->serial);

	if (!signerCert) {
		throw std::runtime_error("Unable to find PKCS9 countersignature certificate");
	}

	for (int i = 0; i < sk_X509_ATTRIBUTE_num(countersignInfo->auth_attr); ++i) {
		X509_ATTRIBUTE* attribute = sk_X509_ATTRIBUTE_value(countersignInfo->auth_attr, i);
		ASN1_OBJECT* attribute_object = X509_ATTRIBUTE_get0_object(attribute);
		ASN1_TYPE* attr_type = X509_ATTRIBUTE_get0_type(attribute, 0);
		/* 
			Note 2 - A countersignature, since it has type SignerInfo, can itself
			contain a countersignature attribute.  Thus it is possible to
			construct arbitrarily long series of countersignatures.
		*/
		if (OBJ_obj2nid(attribute_object) == NID_pkcs9_countersignature) {
			auto data = std::vector<std::uint8_t>(attr_type->value.octet_string->data,
					attr_type->value.octet_string->data + attr_type->value.octet_string->length);
			counterSignatures.emplace_back(data, certificates);
		}
		else if (OBJ_obj2nid(attribute_object) == NID_pkcs9_contentType) {
			continue;
		}

		/* Signing Time (1.2.840.113549.1.9.5) is set to the UTC time of timestamp generation time. */
		else if (OBJ_obj2nid(attribute_object) == NID_pkcs9_signingTime) {
			signingTime = parseDateTime(attr_type->value.utctime);
		}
		/* 
			Message Digest (1.2.840.113549.1.9.4) is set to the hash value of the SignerInfo structure's
			encryptedDigest value. The hash algorithm that is used to calculate the hash value is the same
			as that specified in the SignerInfo structure’s digestAlgorithm value of the timestamp.

			MessageDigest ::= OCTET STRING
		*/
		else if (OBJ_obj2nid(attribute_object) == NID_pkcs9_messageDigest) {
			digest = bytesToHexString(attr_type->value.asn1_string->data, attr_type->value.asn1_string->length);
		}
	}

	PKCS7_SIGNER_INFO_free(countersignInfo);
}

const X509* Pkcs9CounterSignature::getX509() const
{
	return signerCert;
}

} // namespace authenticode