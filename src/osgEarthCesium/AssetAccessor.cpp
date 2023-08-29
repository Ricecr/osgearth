/* -*-c++-*- */
/* osgEarth - Geospatial SDK for OpenSceneGraph
* Copyright 2008-2012 Pelican Mapping
* http://osgearth.org
*
* osgEarth is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
* IN THE SOFTWARE.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>
*/
#include "AssetAccessor"

#include <osgEarth/Notify>
#include <osgEarth/URI>

using namespace osgEarth;
using namespace osgEarth::Cesium;

/**********************************************/
AssetRequest::AssetRequest(const std::string& method, const std::string& url, const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers) :
    _method(method),
    _url(url)
{
    _headers.insert(headers.begin(), headers.end());
}

const std::string& AssetRequest::method() const
{
    return _method;
}

const std::string& AssetRequest::url() const
{
    return _url;
}

const CesiumAsync::HttpHeaders& AssetRequest::headers() const
{
    return _headers;
}

const CesiumAsync::IAssetResponse* AssetRequest::response() const
{
    return _response.get();
}

void AssetRequest::setResponse(std::unique_ptr< AssetResponse > response)
{
    _response = std::move(response);
}

/**********************************************/

uint16_t AssetResponse::statusCode() const
{
    return _statusCode;
}

std::string AssetResponse::contentType() const
{
    return _contentType;
}


const CesiumAsync::HttpHeaders& AssetResponse::headers() const
{
    return _headers;
}

gsl::span<const std::byte> AssetResponse::data() const
{
    return gsl::span<const std::byte>(_result.data(), _result.size());
}

/**********************************************/

CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
AssetAccessor::get(const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& url,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers)
{
    auto request = std::make_shared<AssetRequest>("GET", url, headers);
    return asyncSystem.createFuture<std::shared_ptr<CesiumAsync::IAssetRequest>>(
        [&](const auto& promise)
        {
            asyncSystem.runInWorkerThread([promise, request, url, headers]() {
                // This should run in another thread.
                URIContext uriContext;
                for (auto header : headers)
                {
                    uriContext.addHeader(header.first, header.second);
                }

                URI uri(url, uriContext);
                auto httpResponse = uri.readString();
                std::unique_ptr< AssetResponse > response = std::make_unique< AssetResponse >();

                if (httpResponse.code() == ReadResult::RESULT_OK)
                {
                    response->_statusCode = 200;
                }
                response->_contentType = httpResponse.metadata().value(IOMetadata::CONTENT_TYPE);
                for (auto& i : httpResponse.metadata().children())
                {
                    response->_headers[i.key()] = i.value();
                }
                std::string content = httpResponse.getString();

                std::vector<std::byte> result(content.size());
                for (unsigned int i = 0; i < content.size(); ++i)
                {
                    result[i] = (std::byte)content[i];
                }

                response->_result = result;
                request->setResponse(std::move(response));
                promise.resolve(request);
                });
        }
    );
}

CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
AssetAccessor::request(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& verb,
    const std::string& url,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers,
    const gsl::span<const std::byte>& contentPayload)
{
    auto request = std::make_shared<AssetRequest>(verb, url, headers);
    return asyncSystem.createFuture<std::shared_ptr<CesiumAsync::IAssetRequest>>(
        [&](const auto& promise)
        {
        }
    );
}

void AssetAccessor::tick() noexcept
{
}